// The two turn drives. First choice is the snap: the camera's own one-shot
// desired-orientation request, which turns it within a frame. When the
// active camera never reads that request (the combat camera, an unmapped
// address), the key-hold turn takes over: the player's bound turn key is
// injected as a DirectInput scancode, so the engine's own UpdateCamera runs
// its poll-axis, AcclTurnCamera, apply sequence as if the key were held.
// Calling AcclTurnCamera from outside does not move the chase cam: its
// accumulator is only consumed inside the engine's own per-frame pass.

#include "camera_orient_internal.h"

#include <windows.h>
#include <cmath>

#include "camera_announce.h"  // TryGetCameraEngineYawDegrees
#include "engine_compass.h"
#include "engine_keymap.h"    // TurnScancode: the bound turn key
#include "engine_player.h"
#include "engine_addr.h"
#include "engine_reads.h"
#include "hotkeys.h"
#include "key_inject.h"
#include "log.h"

namespace acc::camera_orient {

namespace {

// ---- Key-hold turn ----------------------------------------------------------
//
// One rotation at a time. Each tick samples yaw and time, projects the time
// to the target and releases the key ahead of arrival: the engine keeps
// turning after the keyup.
struct Rotation {
    bool   active             = false;
    WORD   holdScan           = 0;    // DIK scancode of the held turn key
    char   debugKey           = 0;    // 'L' / 'R', for the log
    float  targetEngineYawRad = 0.0f;
    float  initialAbsDeltaRad = 0.0f;
    DWORD  startedMs          = 0;
    float  prevYawRad         = 0.0f;
    DWORD  prevTickMs         = 0;
};
Rotation g_rot;

// Keyup, DirectInput release, the engine pipeline and the follow cam easing
// out: K1 turns kept going for rate x 75..123 ms after release, and 100 ms
// lands within a few degrees across that spread.
constexpr DWORD kReleaseLookaheadMs = 100;

// The coast after release; the landing line waits this long.
constexpr DWORD kLandSettleMs = 150;
DWORD g_settleUntilMs = 0;
bool  g_landPending   = false;

// Below this (0.57 degrees a second) the time-to-target projection explodes.
constexpr float kMinRateRadPerMs = 1e-5f;

// The engine ignored the key (a load screen, a modal, a scripted takeover):
// never strand it held.
constexpr DWORD kTimeoutMs = 3000;

// ---- Snap ---------------------------------------------------------------------
//
// CSWCameraOnAStick::Control_UpdateCameraDesiredOrientation (K1 0x006391a0,
// K2 0x007dea10, reached from the free-roam camera's Control every update)
// ends with: if (desiredOrientation >= 0) { orientation =
// YawPitchRoll(desiredOrientation, 0, 0); desiredOrientation = -1; }.
// Nothing in the engine writes it, so writing it IS the turn, with no key
// held and no keyboard acceleration ramp (a 150-degree key turn took a
// second). Degrees in the engine's Yaw() frame, atan2(-x, y): North = 0,
// CCW+, our engine frame minus 90. It sits 4 bytes below
// screenFramePercentage (camera_spin_guard) in both games. The combat camera
// never reads it, so the request stays pending.
const uintptr_t kAddrDesiredOrientation = acc::addr::PickGlobal(0x007A2440, 0x00A1090C);

// Still pending after this: this camera does not read the request.
constexpr DWORD kSnapTakeMs = 200;
// Taken but the heading is not there yet (a follow cam that eases): the
// key-hold turn finishes the job after this.
constexpr DWORD kSnapArriveMs = 600;

struct Snap {
    bool        active             = false;
    bool        taken              = false;
    float       targetEngineYawRad = 0.0f;
    DWORD       startedMs          = 0;
    const char* mode               = "";  // a literal
};
Snap g_snap;

bool WriteDesiredOrientation(float deg) {
    return acc::engine::TryWriteFloat(reinterpret_cast<void*>(kAddrDesiredOrientation), 0,
                                      deg);
}

// False on no address or a fault.
bool ReadDesiredOrientationPending(bool& pending) {
    float desired = 0.0f;
    if (!acc::engine::TryReadFloat(reinterpret_cast<void*>(kAddrDesiredOrientation), 0,
                                   &desired)) {
        return false;
    }
    pending = desired >= 0.0f;
    return true;
}

void ReleaseAndDisarm(const char* reason, float curYawRad) {
    if (!g_rot.active) return;
    acc::key_inject::Send(g_rot.holdScan, /*down=*/false);
    // camera_announce speaks the final direction once IsActive() drops.
    const float curCompassDeg = acc::engine::EngineYawToCompass(curYawRad * kRadToDeg);
    acclog::Write("CameraOrient", "release: reason=%s dir=%c cur=%.1f° (compass=%.1f° "
                  "sector=%d) target=%.1f° elapsed=%ums",
                  reason, g_rot.debugKey, curYawRad * kRadToDeg, curCompassDeg,
                  acc::engine::CompassToSector(curCompassDeg),
                  g_rot.targetEngineYawRad * kRadToDeg, GetTickCount() - g_rot.startedMs);
    g_rot.active = false;
    g_settleUntilMs = GetTickCount() + kLandSettleMs;
    g_landPending = true;
}

// False when already at the target (no key to hold).
bool ArmKeyTurn(float targetEngineYawRad, float curYawRad, const char* mode) {
    const float delta = NormaliseRad(targetEngineYawRad - curYawRad);
    if (std::fabs(delta) <= kArrivalRad) {
        acclog::Write("CameraOrient", "no-op: already at target (delta=%.2f° within tolerance)",
                      delta * kRadToDeg);
        return false;
    }
    // Positive delta is counter-clockwise: turn left.
    const bool isLeft = delta > 0.0f;
    const DWORD nowMs = GetTickCount();
    g_rot.active             = true;
    g_rot.holdScan           = static_cast<WORD>(acc::engine_keymap::TurnScancode(isLeft));
    g_rot.debugKey           = isLeft ? 'L' : 'R';
    g_rot.targetEngineYawRad = targetEngineYawRad;
    g_rot.initialAbsDeltaRad = std::fabs(delta);
    g_rot.startedMs          = nowMs;
    g_rot.prevYawRad         = curYawRad;  // the first rate sample's baseline
    g_rot.prevTickMs         = nowMs;
    acc::key_inject::Send(g_rot.holdScan, /*down=*/true);
    acclog::Write("CameraOrient", "arm: mode=%s cur=%.1f° target=%.1f° delta=%.1f° dir=%c "
                  "(scan=0x%02x)", mode, curYawRad * kRadToDeg,
                  targetEngineYawRad * kRadToDeg, delta * kRadToDeg, g_rot.debugKey,
                  g_rot.holdScan);
    return true;
}

void TickKeyTurn(void* camera) {
    if (!camera) {
        // Area transition or shutdown: never leave the key held across it.
        acc::key_inject::Send(g_rot.holdScan, /*down=*/false);
        acclog::Write("CameraOrient", "release: reason=camera_lost dir=%c", g_rot.debugKey);
        g_rot.active = false;
        g_landPending = false;
        return;
    }
    float curYawRad = 0.0f;
    if (!ReadCurrentEngineYawRad(curYawRad)) {
        ReleaseAndDisarm("getyaw_fault", 0.0f);
        return;
    }
    const float remaining = NormaliseRad(g_rot.targetEngineYawRad - curYawRad);
    const DWORD nowMs = GetTickCount();

    // Project the time to the target from the turn rate, when it turns
    // toward the target at all; otherwise fall back to the arrival window.
    bool  useRate      = false;
    float rateRadPerMs = 0.0f;
    float ttaMs        = 0.0f;
    const DWORD dtMs = nowMs - g_rot.prevTickMs;
    if (dtMs > 0) {
        rateRadPerMs = NormaliseRad(curYawRad - g_rot.prevYawRad) / static_cast<float>(dtMs);
        if (std::fabs(rateRadPerMs) >= kMinRateRadPerMs &&
            (remaining > 0.0f) == (rateRadPerMs > 0.0f)) {
            ttaMs = remaining / rateRadPerMs;
            useRate = true;
        }
    }
    const bool arrived = useRate ? ttaMs <= static_cast<float>(kReleaseLookaheadMs)
                                 : std::fabs(remaining) <= kArrivalRad;
    // Grown past the start plus 45 degrees: it sailed the wrong way (the
    // engine ignored the key and the player turned the other way).
    const bool overshot = std::fabs(remaining) > g_rot.initialAbsDeltaRad + kPi * 0.25f;
    const bool timedOut = nowMs - g_rot.startedMs >= kTimeoutMs;
    g_rot.prevYawRad = curYawRad;
    g_rot.prevTickMs = nowMs;
    if (!arrived && !overshot && !timedOut) return;

    const char* reason = arrived ? "arrived" : overshot ? "overshot" : "timeout";
    acclog::Write("CameraOrient", "decide: %s remaining=%.2f° rate=%.3f°/ms tta=%.0fms "
                  "(useRate=%d)", reason, remaining * kRadToDeg, rateRadPerMs * kRadToDeg,
                  ttaMs, useRate ? 1 : 0);
    ReleaseAndDisarm(reason, curYawRad);
}

// False when the request cannot be written (no address on this build, or a
// fault).
bool StartSnap(float targetEngineYawRad, float curYawRad, const char* mode) {
    float deg = std::fmod(targetEngineYawRad * kRadToDeg - 90.0f, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    if (!WriteDesiredOrientation(deg)) return false;
    g_snap = Snap{true, false, targetEngineYawRad, GetTickCount(), mode};
    acclog::Write("CameraOrient", "snap: mode=%s cur=%.1f° target=%.1f° delta=%.1f° "
                  "request=%.1f°", mode, curYawRad * kRadToDeg, targetEngineYawRad * kRadToDeg,
                  NormaliseRad(targetEngineYawRad - curYawRad) * kRadToDeg, deg);
    return true;
}

// Waits for the camera to take the request, then for the heading to show
// it; either wait running out hands the rest to the key-hold turn.
void TickSnap(void* camera) {
    const DWORD elapsed = GetTickCount() - g_snap.startedMs;
    if (!camera) {
        WriteDesiredOrientation(-1.0f);
        acclog::Write("CameraOrient", "snap: camera lost after %ums", elapsed);
        g_snap.active = false;
        return;
    }
    if (!g_snap.taken) {
        bool pending = true;
        if (ReadDesiredOrientationPending(pending) && !pending) {
            g_snap.taken = true;
            acclog::Write("CameraOrient", "snap: taken after %ums", elapsed);
        } else if (elapsed < kSnapTakeMs) {
            return;
        } else {
            // Withdrawn: taken later, it would yank the camera in the middle
            // of the key turn that replaces it.
            WriteDesiredOrientation(-1.0f);
            g_snap.active = false;
            acclog::Write("CameraOrient", "snap: not taken in %ums (combat camera?) — key turn",
                          elapsed);
            float curYawRad = 0.0f;
            if (ReadCurrentEngineYawRad(curYawRad)) {
                ArmKeyTurn(g_snap.targetEngineYawRad, curYawRad, g_snap.mode);
            }
            return;
        }
    }
    float curYawRad = 0.0f;
    if (!ReadCurrentEngineYawRad(curYawRad)) {
        acclog::Write("CameraOrient", "snap: yaw read faulted after %ums", elapsed);
        g_snap.active = false;
        return;
    }
    const float remaining = NormaliseRad(g_snap.targetEngineYawRad - curYawRad);
    if (std::fabs(remaining) <= kArrivalRad) {
        acclog::Write("CameraOrient", "snap landed: cur=%.1f° target=%.1f° error=%.1f° "
                      "elapsed=%ums", curYawRad * kRadToDeg,
                      g_snap.targetEngineYawRad * kRadToDeg, -remaining * kRadToDeg, elapsed);
        g_snap.active = false;
        return;
    }
    if (elapsed >= kSnapArriveMs) {
        acclog::Write("CameraOrient", "snap: taken but still %.1f° off after %ums — key turn "
                      "for the rest", remaining * kRadToDeg, elapsed);
        g_snap.active = false;
        ArmKeyTurn(g_snap.targetEngineYawRad, curYawRad, g_snap.mode);
    }
}

}  // namespace

// camera_announce's position-derived yaw first: atan2(player - camera) is
// single-valued. The quaternion (engine_player.h) is the fallback for when
// that degenerates (camera straight above the player) or has not anchored.
bool ReadCurrentEngineYawRad(float& out) {
    float deg = 0.0f;
    if (acc::camera_announce::TryGetCameraEngineYawDegrees(deg)) {
        out = deg * kDegToRad;
        return true;
    }
    return acc::engine::GetCameraYawRadians(out);
}

bool TurnInFlight() {
    return g_rot.active || g_snap.active;
}

void TickTurn(void* camera) {
    if (g_snap.active) {
        TickSnap(camera);
    } else if (g_rot.active) {
        TickKeyTurn(camera);
    }
}

void StartTurn(float targetEngineYawRad, float curYawRad, const char* mode) {
    if (!StartSnap(targetEngineYawRad, curYawRad, mode)) {
        ArmKeyTurn(targetEngineYawRad, curYawRad, mode);
    }
}

// One line per key-hold turn once the coast is over: where the camera really
// stopped. The release line cannot show it, as the camera still moves then.
void LogLanding() {
    if (!g_landPending || static_cast<int>(GetTickCount() - g_settleUntilMs) < 0) return;
    g_landPending = false;
    float curYawRad = 0.0f;
    if (!ReadCurrentEngineYawRad(curYawRad)) return;
    acclog::Write("CameraOrient", "landed: cur=%.1f° target=%.1f° error=%.1f°",
                  curYawRad * kRadToDeg, g_rot.targetEngineYawRad * kRadToDeg,
                  NormaliseRad(curYawRad - g_rot.targetEngineYawRad) * kRadToDeg);
}

// Also true on the press's own tick, before a turn is armed: camera_announce
// ticks first in core_tick and would speak the old sector on that frame.
bool IsActive() {
    return TurnInFlight() || acc::hotkeys::Held(acc::hotkeys::Action::CameraOrient);
}

}  // namespace acc::camera_orient
