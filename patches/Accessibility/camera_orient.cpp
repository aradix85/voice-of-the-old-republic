#include "camera_orient_internal.h"

#include <windows.h>
#include <cmath>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")

#include "camera_announce.h"  // TryGetCameraEngineYawDegrees
#include "engine_app.h"      // GetCamera
#include "engine_area.h"     // GetCurrentArea
#include "engine_compass.h"
#include "engine_keymap.h"    // TurnScancode — bound turn key
#include "engine_offsets.h"
#include "engine_player.h"
#include "engine_rebase.h"    // PickGlobal — the desired-orientation request
#include "hotkeys.h"
#include "key_inject.h"       // Send — the shared scancode injection
#include "log.h"
#include "room_topology.h"   // LookupAt — the region's exits and their points

namespace acc::camera_orient {

namespace {

float CompassDegToEngineRad(float compassDeg) {
    float engineDeg = std::fmod(90.0f - compassDeg + 360.0f, 360.0f);
    if (engineDeg < 0.0f) engineDeg += 360.0f;
    return engineDeg * kDegToRad;
}

// The exit N last turned to, so the next press in the same region cycles
// on instead of re-aiming at it. Only trusted while the player is still in
// that region and still faces it; walking into another region retires it.
struct LastExit {
    bool   valid     = false;
    void*  area      = nullptr;
    int    clusterId = acc::room_topology::kClusterIdNone;
    Vector pos       = {0.0f, 0.0f, 0.0f};
};
LastExit g_lastExit;

// "Still facing the exit N turned to": the release lands a few degrees
// short or long (predictive release + coast), so exact equality never holds.
constexpr float kStillFacingRad = 10.0f * 0.017453292519943295f;

// An exit point this close to the player has no usable bearing.
constexpr float kMinExitDistM = 0.5f;

// Exits of the player's region as engine-frame bearings (0=East, CCW+),
// measured from where the player stands — the real direction, not the
// octant word the label speaks. Bearings within the arrival tolerance of
// one already kept collapse into it: a turn between them would be a no-op.
struct ExitBearing {
    float  yawRad;
    Vector pos;
};

bool CollectExitBearings(const Vector& playerPos, void*& outArea,
                         int& outClusterId, std::vector<ExitBearing>& out) {
    out.clear();
    outArea = acc::engine::GetCurrentArea();
    outClusterId = acc::room_topology::kClusterIdNone;
    if (!outArea) return false;
    std::string label;
    int sig = 0;
    std::vector<acc::room_topology::ExitMark> exits;
    if (!acc::room_topology::LookupAt(outArea, playerPos, label, sig,
                                      outClusterId,
                                      /*allowDiagLog=*/false,
                                      /*requireWallReachable=*/true,
                                      /*zTrusted=*/true, &exits)) {
        return false;
    }
    for (const auto& e : exits) {
        float dx = e.pos.x - playerPos.x;
        float dy = e.pos.y - playerPos.y;
        if (dx * dx + dy * dy < kMinExitDistM * kMinExitDistM) continue;
        float yaw = std::atan2(dy, dx);
        bool dup = false;
        for (const auto& k : out) {
            if (std::fabs(NormaliseRad(yaw - k.yawRad)) <= kFallbackArrivalRad) {
                dup = true;
                break;
            }
        }
        if (!dup) out.push_back({yaw, e.pos});
    }
    return !out.empty();
}

// Next exit clockwise from `fromYawRad` (engine frame is CCW+, so
// clockwise = decreasing yaw). Never `fromIdx` itself unless it is the
// only exit.
int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx) {
    int best = fromIdx;
    float bestCw = kTwoPi + 1.0f;
    for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
        if (i == fromIdx) continue;
        float cw = std::fmod(exits[fromIdx].yawRad - exits[i].yawRad + 2.0f * kTwoPi,
                             kTwoPi);
        if (cw < bestCw) {
            bestCw = cw;
            best = i;
        }
    }
    return best;
}

// Pick the exit to turn to:
//   - still in the region of the last N turn and still facing that exit →
//     the next one clockwise (the cycle);
//   - otherwise the exit nearest the camera heading, however small the
//     offset — N lines the player up exactly. Only when that exit is
//     already within the arrival tolerance (nothing to turn) does it move
//     on clockwise.
int ChooseExit(const std::vector<ExitBearing>& exits, void* area,
               int clusterId, float curYawRad, const char*& outWhy) {
    if (g_lastExit.valid && g_lastExit.area == area &&
        g_lastExit.clusterId == clusterId) {
        for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
            if (DistXYSq(exits[i].pos, g_lastExit.pos) > 0.01f) continue;
            if (std::fabs(NormaliseRad(exits[i].yawRad - curYawRad)) <=
                kStillFacingRad) {
                outWhy = "cycle";
                return NextExitClockwise(exits, i);
            }
            break;
        }
    }
    int nearest = 0;
    float nearestAbs = kTwoPi;
    for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
        float a = std::fabs(NormaliseRad(exits[i].yawRad - curYawRad));
        if (a < nearestAbs) {
            nearestAbs = a;
            nearest = i;
        }
    }
    if (nearestAbs <= kFallbackArrivalRad) {
        outWhy = "already-facing";
        return NextExitClockwise(exits, nearest);
    }
    outWhy = "nearest";
    return nearest;
}

// Next cardinal in CW order (N → E → S → W → N).
float NextCardinalCompassDeg(float currentCompassDeg) {
    if (currentCompassDeg < 0.0f) {
        currentCompassDeg = std::fmod(currentCompassDeg, 360.0f) + 360.0f;
    }
    if (currentCompassDeg >= 360.0f) {
        currentCompassDeg = std::fmod(currentCompassDeg, 360.0f);
    }
    int curIdx = static_cast<int>(currentCompassDeg / 90.0f);
    int nextIdx = (curIdx + 1) % 4;
    return static_cast<float>(nextIdx) * 90.0f;
}
}  // namespace

void Tick() {
    void* camera = acc::engine::GetCamera();

    // In-flight turn: tick whichever mechanism is driving it.
    if (g_snap.active || g_rot.active) {
        if (g_snap.active) TickSnap(camera);
        else               TickKeyTurn(camera);
        // A snap lands in a frame or two, so a press can only fall inside a
        // turn on the key-hold fallback; it is dropped there.
        if (acc::hotkeys::Pressed(acc::hotkeys::Action::CameraOrient)) {
            acclog::Write("CameraOrient",
                          "ignore re-press: rotation in flight");
        }
        return;
    }

    LogLanding();

    // Rising edge — arm a new rotation.
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::CameraOrient)) return;

    if (!camera) {
        acclog::Write("CameraOrient", "skip: no camera (main menu / pre-spawn)");
        return;
    }
    float curYawRad = 0.0f;
    if (!ReadCurrentEngineYawRad(curYawRad)) {
        acclog::Write("CameraOrient", "skip: Camera::GetYaw call faulted");
        return;
    }

    const char* mode = "cardinal";
    float targetEngineYawRad = 0.0f;
    Vector playerPos = {0.0f, 0.0f, 0.0f};
    void* exitArea = nullptr;
    int exitCluster = acc::room_topology::kClusterIdNone;
    std::vector<ExitBearing> exits;

    if (acc::engine::GetPlayerPosition(playerPos) &&
        CollectExitBearings(playerPos, exitArea, exitCluster, exits)) {
        // Face the region's exits — the real bearing from where the player
        // stands, so a press or two lines the walk up with where to go.
        const char* why = "nearest";
        int pick = ChooseExit(exits, exitArea, exitCluster, curYawRad, why);
        targetEngineYawRad = exits[pick].yawRad;
        g_lastExit.valid     = true;
        g_lastExit.area      = exitArea;
        g_lastExit.clusterId = exitCluster;
        g_lastExit.pos       = exits[pick].pos;
        mode = "exit";
        acclog::Write("CameraOrient",
                      "exit pick: %s idx=%d of %d cluster=%d bearing=%.1f°",
                      why, pick, static_cast<int>(exits.size()), exitCluster,
                      targetEngineYawRad * kRadToDeg);
    } else {
        // No region or no exit with a usable bearing (open-area fallback,
        // graph not built): the old compass step.
        g_lastExit.valid = false;
        mode = "cardinal";
        float curCompassDeg = acc::engine::EngineYawToCompass(
            curYawRad * kRadToDeg);
        float targetCompassDeg = NextCardinalCompassDeg(curCompassDeg);
        targetEngineYawRad = CompassDegToEngineRad(targetCompassDeg);
    }

    float delta = NormaliseRad(targetEngineYawRad - curYawRad);
    if (std::fabs(delta) <= kFallbackArrivalRad) {
        acclog::Write("CameraOrient",
                      "no-op: already at target (delta=%.2f° within "
                      "tolerance)",
                      delta * kRadToDeg);
        return;
    }

    // The snap first; the key-hold turn when this build cannot write it.
    if (StartSnap(targetEngineYawRad, curYawRad, mode)) return;
    ArmKeyTurn(targetEngineYawRad, curYawRad, mode);
}

}  // namespace acc::camera_orient
