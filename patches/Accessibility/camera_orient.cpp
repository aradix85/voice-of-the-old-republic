// The N key: turns the camera to the exits of the player's region one by
// one (the nearest first, then clockwise), or to the next cardinal where the
// region has none. What to face is decided here (the rules are engine-free
// in camera_orient_pick.cpp); camera_orient_turn.cpp drives the turn.

#include "camera_orient_internal.h"

#include <cmath>
#include <string>
#include <vector>

#include "engine_app.h"      // GetCamera
#include "engine_area.h"     // GetCurrentArea
#include "engine_compass.h"
#include "engine_player.h"
#include "hotkeys.h"
#include "log.h"
#include "room_topology.h"   // LookupAt: the region's exits and their points

namespace acc::camera_orient {

namespace {

// An exit point this close to the player has no usable bearing.
constexpr float kMinExitDistM = 0.5f;

// The exit the last press turned to, trusted while the player is still in
// that region; walking into another region retires it.
struct LastExit {
    bool   valid     = false;
    void*  area      = nullptr;
    int    clusterId = acc::room_topology::kClusterIdNone;
    Vector pos       = {0.0f, 0.0f, 0.0f};
};
LastExit g_lastExit;

// The region's exits as bearings from where the player stands: the real
// direction, not the octant word the label speaks.
bool CollectExitBearings(const Vector& playerPos, void*& outArea, int& outClusterId,
                         std::vector<ExitBearing>& out) {
    out.clear();
    outArea = acc::engine::GetCurrentArea();
    outClusterId = acc::room_topology::kClusterIdNone;
    if (!outArea) return false;
    std::string label;
    int sig = 0;
    std::vector<acc::room_topology::ExitMark> exits;
    if (!acc::room_topology::LookupAt(outArea, playerPos, label, sig, outClusterId,
                                      /*allowDiagLog=*/false,
                                      /*requireWallReachable=*/true,
                                      /*zTrusted=*/true, &exits)) {
        return false;
    }
    for (const auto& e : exits) {
        if (DistXYSq(e.pos, playerPos) < kMinExitDistM * kMinExitDistM) continue;
        AddExitBearing(out, {std::atan2(e.pos.y - playerPos.y, e.pos.x - playerPos.x), e.pos});
    }
    return !out.empty();
}

int LastExitIndex(const std::vector<ExitBearing>& exits, void* area, int clusterId) {
    if (!g_lastExit.valid || g_lastExit.area != area || g_lastExit.clusterId != clusterId) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
        if (DistXYSq(exits[i].pos, g_lastExit.pos) <= 0.01f) return i;
    }
    return -1;
}

// The heading this press turns to; `mode` names the rule for the log.
float ChooseTarget(float curYawRad, const char*& mode) {
    Vector playerPos = {0.0f, 0.0f, 0.0f};
    void* area = nullptr;
    int cluster = acc::room_topology::kClusterIdNone;
    std::vector<ExitBearing> exits;
    if (acc::engine::GetPlayerPosition(playerPos) &&
        CollectExitBearings(playerPos, area, cluster, exits)) {
        const char* why = "nearest";
        const int pick = ChooseExit(exits, LastExitIndex(exits, area, cluster), curYawRad, why);
        g_lastExit = LastExit{true, area, cluster, exits[pick].pos};
        mode = "exit";
        acclog::Write("CameraOrient", "exit pick: %s idx=%d of %d cluster=%d bearing=%.1f°",
                      why, pick, static_cast<int>(exits.size()), cluster,
                      exits[pick].yawRad * kRadToDeg);
        return exits[pick].yawRad;
    }
    // No region, or no exit with a usable bearing (open areas, graph not
    // built): the compass step.
    g_lastExit.valid = false;
    mode = "cardinal";
    const float curCompassDeg = acc::engine::EngineYawToCompass(curYawRad * kRadToDeg);
    return CompassDegToEngineRad(NextCardinalCompassDeg(curCompassDeg));
}

}  // namespace

void Tick() {
    void* camera = acc::engine::GetCamera();
    if (TurnInFlight()) {
        TickTurn(camera);
        // Only the key-hold turn lasts long enough to catch a press.
        if (acc::hotkeys::Pressed(acc::hotkeys::Action::CameraOrient)) {
            acclog::Write("CameraOrient", "ignore re-press: rotation in flight");
        }
        return;
    }
    LogLanding();

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
    const char* mode = "";
    const float target = ChooseTarget(curYawRad, mode);
    const float delta = NormaliseRad(target - curYawRad);
    if (std::fabs(delta) <= kArrivalRad) {
        acclog::Write("CameraOrient", "no-op: already at target (delta=%.2f° within tolerance)",
                      delta * kRadToDeg);
        return;
    }
    StartTurn(target, curYawRad, mode);
}

}  // namespace acc::camera_orient
