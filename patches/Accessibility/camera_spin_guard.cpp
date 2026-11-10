#include "camera_spin_guard.h"

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "camera_orient.h"   // IsActive — our own snap-turn drives the camera
#include "engine_app.h"      // GetClientAppInternal — K2 viewport-object chain
#include "engine_input.h"    // PollTurnAxis / ReadNativeStickAxes — the two
                             // sources UpdateCamera feeds to AcclTurnCamera
#include "engine_keymap.h"   // AnyMovementKeyHeld — the bound move/turn keys
#include "engine_manager.h"  // kAddrGuiManagerPtr
#include "engine_options.h"  // GetMouseLook
#include "engine_panels.h"   // IsForegroundUiBlocking — cinematic / screen gate
#include "engine_player.h"   // GetCameraYawRadians / GetCameraPosition /
                             // GetPlayerPosition + chain constants
#include "core_tick.h"       // NowMs — the rate below divides by an inter-tick
                             // interval, which GetTickCount cannot resolve
#include "hotkeys.h"         // IsForegroundGame — guard only when focused
#include "log.h"
#include "engine_offsets_select.h"
#include "engine_rebase.h"

#pragma comment(lib, "user32.lib")

namespace acc::camera_spin_guard {

namespace {

constexpr float kRadToDeg = 57.29577951308232f;

// Engine globals (decompiled, see UpdateCamera @0x5f5e10).
// K2 witnessed 2026-08-02 in the K2 edge-band code at 0x0078C520-0x0078C6A0
// (found via the 0.001f-initialised .data global): screenFramePercentage
// twin = 0x00A10910 (both FMULs), mouse_x read as [[GuiManager global]+0]
// (mouse_y +4), viewport width as int16 [+0x6c] on the object at
// [client internal + 0x274] — K2 splits the width off the GuiManager.
const uintptr_t kAddrScreenFramePercent   = acc::addr::PickGlobal(0x007A2444, 0x00A10910);  // float
// rightClickHeld: LOG-ONLY (episode START diagnostic). No K2 witness yet —
// SafeRead declines to 0 there. Not part of the guard's firing logic.
// K2 value witnessed 2026-09-14 in the UpdateCamera twin @0x0078c350: read into
// the edge-turn gate, XORed with the mouse_look option bit exactly as K1 does.
const uintptr_t kAddrRightClickHeld       = acc::addr::PickGlobal(0x008338F0, 0x00A7FCBC);  // int
const size_t    kGuiMouseXOffset          = acc::off::Same(0x00);        // ulong
const size_t    kGuiMouseYOffset          = acc::off::Same(0x04);        // ulong
// K2 only (AcclTurnCamera twin @0x007f4220, follow-cam branch): the module
// mirrors the follow behaviour's turn accumulator to +0x98 and raises +0x100
// whenever it applies a non-zero turn. Both are witnesses that the camera's
// OWN turn path ran; a spin that leaves them untouched came from elsewhere
// (the player creature rotating under the chase camera, most likely). On K1
// +0x98 is not the accumulator (camera-and-swoop.md), so these stay K2-only.
const size_t    kK2ModuleAcclMirrorOffset = 0x98;   // float
const size_t    kK2ModuleTurnedFlagOffset = 0x100;  // int
const size_t    kGuiViewportWidthOffset   = acc::off::Same(0x6C);        // int16
// K2 only: the viewport object holding width/height (+0x6c/+0x6e) hangs off
// the client internal at +0x274 (witnessed via the getter 0x0073FEA0). On
// K1 the width lives on the GuiManager itself, so this is unused there.
const size_t    kK2InternalViewportObjOffset = 0x274;

// Cursor-edge guard (Option A): after a load the cursor parks frozen at the
// screen border (mouse_x=0) and the post-load input pipeline ignores
// injected motion (SetCursorPos and SendInput both failed to move it — see
// logs 124537/131734). Since nothing is actively re-setting mouse_x, we
// write the engine cursor position directly to viewport centre, which
// bypasses the broken input path and takes the cursor out of the edge band.
// If the engine turns out to re-derive mouse_x each frame, the diagnostic
// will show it bouncing back to 0 and we move the clamp into a UpdateCamera
// detour instead.

// Guard fires once the camera is actually rotating, so it stays quiet when
// the camera is genuinely static (e.g. a paused menu with the cursor parked
// at an edge). 3 deg/s catches the onset before AcclTurnCamera ramps up.
constexpr float kGuardRateThresh = 3.0f;

// Logging is episode-based, not per-frame: one START line when an edge-guard
// episode begins, one END summary when the cursor has stayed clear of the
// band this long. Collapses a multi-second bounce (hundreds of frames) to
// two lines.
constexpr DWORD kEpisodeQuietMs = 500;

// Sanity tripwire for the camera-yaw read (fix #1). quat- and position-
// derived yaw agreed to 0.0 in bench testing, so a divergence CAN mean
// GetCameraYawRadians regressed — but only under the gates at the call site.
// posYaw is the camera->player BEARING, which equals the camera's facing only
// while the camera sits in its normal follow pose. Ungated, this fired on
// every cinematic shot, every camera lag during a turn and every combat orbit:
// 319 lines across 25 K1 dev sessions at a 136 deg median, and 235 in one
// field session — noise that reads like a regression and is not one.
constexpr float kReadAnomalyDeg = 2.0f;

// A regression is PERSISTENT; legitimate divergence is transient. Require the
// divergence to hold continuously for this long, with every gate satisfied,
// before saying anything.
constexpr DWORD kAnomalyPersistMs = 3000;
constexpr DWORD kAnomalyRepeatMs  = 10000;

// Plausible chase-cam distance to the player, metres. Outside this the camera
// is not in its follow pose (scripted shot, mid-transition) and the bearing
// cross-check means nothing. Deliberately generous — the band exists to
// exclude far cinematic framing, not to tune out real divergence — and the
// measured distance is logged so field data can narrow it.
constexpr float kFollowDistMinM = 1.0f;
constexpr float kFollowDistMaxM = 15.0f;

// ---- Uncommanded-rotation watch -------------------------------------------
// Rotation slower than this is settle/drift, not a spin.
constexpr float kSpinRateThresh = 20.0f;   // deg/s
// Report every full turn the camera makes with no input behind it. One turn is
// unambiguous: no legitimate engine behaviour rotates a player-controlled
// camera 360 degrees while the player commands nothing.
constexpr float kSpinReportDeg  = 360.0f;
// Stop logging after this many turns in one episode — a runaway is established
// by then and the log has the state it needs.
constexpr int   kSpinMaxReports = 5;

template <typename T>
T SafeRead(uintptr_t addr, T fallback) {
    __try {
        return *reinterpret_cast<T*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return fallback;
    }
}

template <typename T>
T SafeReadOff(void* base, size_t off, T fallback) {
    if (!base) return fallback;
    __try {
        return *reinterpret_cast<T*>(
            reinterpret_cast<unsigned char*>(base) + off);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return fallback;
    }
}

template <typename T>
bool SafeWriteOff(void* base, size_t off, T value) {
    if (!base) return false;
    __try {
        *reinterpret_cast<T*>(
            reinterpret_cast<unsigned char*>(base) + off) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

float NormDeg(float d) {
    d = std::fmod(d, 360.0f);
    if (d < 0.0f) d += 360.0f;
    return d;
}

// Signed shortest delta a-b in (-180, 180].
float AngularDelta(float a, float b) {
    return std::fmod(a - b + 540.0f, 360.0f) - 180.0f;
}

struct State {
    bool  have       = false;
    float prevYaw    = 0.0f;   // quaternion-derived, deg
    DWORD prevMs     = 0;
    bool  havePos    = false;
    float prevPosYaw = 0.0f;   // camera->player bearing, deg
    DWORD prevPosMs  = 0;
    bool  havePlayer = false;
    float prevPlayerYaw = 0.0f;  // player creature facing, deg
    DWORD prevPlayerMs  = 0;
};
State g_state;

// One edge-guard "episode" = a contiguous bout of edge contact (with brief
// corrected gaps). Logged as START + END-summary instead of per frame.
struct Episode {
    bool  active      = false;
    DWORD startMs     = 0;
    DWORD lastEdgeMs  = 0;
    int   corrections = 0;
    int   edgeFrames  = 0;
    float maxRate     = 0.0f;
    float startYaw    = 0.0f;
};
Episode g_ep;

// ---- Read-anomaly tripwire state (see the gate note at the call site) ------
DWORD g_anomalySinceMs = 0;   // when the CURRENT divergence run began; 0 = none
DWORD g_lastAnomalyMs  = 0;   // last line emitted, for the repeat throttle
bool  g_anomalyOpen    = false;

// ---- Uncommanded-rotation watch state -------------------------------------
// Cause-agnostic: accumulates camera rotation while the player is commanding
// none. The edge-guard above only knows ONE cause (cursor in the edge band);
// this notices the symptom whatever produced it.
struct SpinWatch {
    bool  active      = false;
    DWORD startMs     = 0;
    float accumQuat   = 0.0f;  // signed, deg — from the orientation quaternion
    float accumPos    = 0.0f;  // signed, deg — from the camera->player bearing
    float accumPlayer = 0.0f;  // signed, deg — the player creature's own facing
    float maxRate     = 0.0f;
    int   frames      = 0;
    int   reports     = 0;     // full turns already reported this episode
};
SpinWatch g_spin;

void ResetSpinWatch() {
    g_spin.active    = false;
    g_spin.accumQuat = 0.0f;
    g_spin.accumPos  = 0.0f;
    g_spin.accumPlayer = 0.0f;
    g_spin.maxRate   = 0.0f;
    g_spin.frames    = 0;
    g_spin.reports   = 0;
}

// Close the watch, summarising only episodes worth a line.
//
// Releasing a turn key ends "commanded" a moment before the chase-cam finishes
// catching up, so a short uncommanded rotation is NORMAL and must stay silent —
// otherwise every turn the player makes ends in a log line. A reported episode
// always summarises; an unreported one only if it ran long AND swept far enough
// that settle cannot explain it.
constexpr DWORD kSpinSummaryMs  = 2000;
constexpr float kSpinSummaryDeg = 90.0f;

void EndSpinWatch(DWORD now, const char* why) {
    if (g_spin.active) {
        DWORD dur = now - g_spin.startMs;
        float swept = std::fabs(g_spin.accumQuat) > std::fabs(g_spin.accumPos)
                          ? std::fabs(g_spin.accumQuat)
                          : std::fabs(g_spin.accumPos);
        if (g_spin.reports > 0 ||
            (dur >= kSpinSummaryMs && swept >= kSpinSummaryDeg)) {
            acclog::Write("CameraSpinDiag",
                "uncommanded-rotation END: duration=%ums frames=%d "
                "quatTurn=%.0f deg posTurn=%.0f deg playerTurn=%.0f deg "
                "maxRate=%.0f deg/s (ended: %s)",
                dur, g_spin.frames, g_spin.accumQuat, g_spin.accumPos,
                g_spin.accumPlayer, g_spin.maxRate, why);
        }
    }
    ResetSpinWatch();
}

void EndEpisode(DWORD now, float endYaw) {
    acclog::Write("CameraSpinDiag",
        "edge-guard END: corrections=%d edgeFrames=%d duration=%ums "
        "maxRate=%.0f deg/s netYaw=%.0f->%.0f",
        g_ep.corrections, g_ep.edgeFrames, now - g_ep.startMs,
        g_ep.maxRate, g_ep.startYaw, endYaw);
    g_ep.active = false;
}

// True while something other than the player is entitled to move the camera:
// an engine cinematic / dialog shot, a blocking screen, or our own N-hotkey
// snap-turn. Both the tripwire and the spin watch are meaningless then — the
// camera is legitimately somewhere the player did not put it.
//
// IsForegroundUiBlocking already tests HasActiveDialogPanel first (it is the
// predicate's own rule 1), so the cinematic case needs no separate scan.
bool CameraDrivenByNonPlayer() {
    return acc::engine::IsForegroundUiBlocking() ||
           acc::camera_orient::IsActive();
}

// One tick's readings: the three yaw witnesses and the cursor against the
// edge band.
struct Sample {
    float    quatYawDeg     = -1.0f;  // orientation quaternion; -1 = unreadable
    float    posYawDeg      = -1.0f;  // camera->player bearing; -1 = unreadable
    float    camDistM       = -1.0f;
    float    rateDegPerSec  = 0.0f;
    bool     haveRate       = false;
    float    quatStepDeg    = 0.0f;   // signed rotation since the last sample
    float    posStepDeg     = 0.0f;
    float    posRateDps     = 0.0f;
    bool     havePosStep    = false;
    float    playerYawDeg   = -1.0f;
    float    playerStepDeg  = 0.0f;
    float    playerRateDps  = 0.0f;
    bool     havePlayerStep = false;
    void*    gui            = nullptr;
    uint32_t mouseX         = 0;
    uint32_t mouseY         = 0;
    int16_t  vpW            = 0;
    int      bandPx         = 0;
    bool     edgeNow        = false;
    const char* edge        = "none";
};

// A yaw witness's step since its last sample. A zero-length interval HOLDS
// the previous sample: a tick is one rendered frame, shorter than the clock
// can always resolve, and a measurement not yet taken is not a measurement
// of zero (that once made the guard skip a correction on a spinning frame).
// The tick clock (QPC-backed) times it.
bool StepWitness(float yawDeg, DWORD now, bool& have, float& prevYaw, DWORD& prevMs,
                 float& stepDeg, float& rateDps) {
    bool step = false;
    const DWORD dt = now - prevMs;
    if (have && dt > 0) {
        stepDeg = AngularDelta(yawDeg, prevYaw);
        rateDps = stepDeg * 1000.0f / static_cast<float>(dt);
        step = true;
    }
    if (step || !have) {
        prevYaw = yawDeg;
        prevMs  = now;
        have    = true;
    }
    return step;
}

// The three witnesses. The bearing also orbits during an edge-turn, so a
// dead quaternion read cannot hide a spin; it is tracked independently of
// the quaternion sample for exactly that reason. The player's own facing
// separates a creature turning under the chase cam (player rate matches the
// camera's: a server-side driver such as facing snaps, AI, pushing) from the
// camera's own turn path (player rate near zero), which the two camera
// witnesses alone cannot tell apart.
void SampleYaws(DWORD now, const Vector& playerPos, Sample& s) {
    float r = 0.0f;
    if (acc::engine::GetCameraYawRadians(r)) s.quatYawDeg = NormDeg(r * kRadToDeg);
    Vector camPos;
    if (acc::engine::GetCameraPosition(camPos)) {
        const float dx = playerPos.x - camPos.x;
        const float dy = playerPos.y - camPos.y;
        s.camDistM = std::sqrt(dx * dx + dy * dy);
        if (s.camDistM >= 0.1f) s.posYawDeg = NormDeg(std::atan2(dy, dx) * kRadToDeg);
    }
    if (s.quatYawDeg >= 0.0f) {
        s.haveRate = StepWitness(s.quatYawDeg, now, g_state.have, g_state.prevYaw, g_state.prevMs,
                                 s.quatStepDeg, s.rateDegPerSec);
    }
    if (s.posYawDeg >= 0.0f) {
        s.havePosStep = StepWitness(s.posYawDeg, now, g_state.havePos, g_state.prevPosYaw,
                                    g_state.prevPosMs, s.posStepDeg, s.posRateDps);
    }
    float y = 0.0f;
    if (acc::engine::GetPlayerYawDegrees(y)) s.playerYawDeg = NormDeg(y);
    if (s.playerYawDeg >= 0.0f) {
        s.havePlayerStep = StepWitness(s.playerYawDeg, now, g_state.havePlayer,
                                       g_state.prevPlayerYaw, g_state.prevPlayerMs,
                                       s.playerStepDeg, s.playerRateDps);
    }
}

// The cursor against the edge band. The width lives on the GuiManager in K1
// and on the viewport object at [client internal + 0x274] in K2.
void SampleCursor(Sample& s) {
    s.gui = SafeRead<void*>(kAddrGuiManagerPtr, nullptr);
    s.mouseX = SafeReadOff<uint32_t>(s.gui, kGuiMouseXOffset, 0);
    s.mouseY = SafeReadOff<uint32_t>(s.gui, kGuiMouseYOffset, 0);
    void* vpObj = s.gui;
    if (acc::game::IsKotor2()) {
        vpObj = SafeReadOff<void*>(acc::engine::GetClientAppInternal(),
                                   kK2InternalViewportObjOffset, nullptr);
    }
    s.vpW = SafeReadOff<int16_t>(vpObj, kGuiViewportWidthOffset, 0);
    const float pct = SafeRead<float>(kAddrScreenFramePercent, 0.0f);
    // The engine's real turn band is wider than screenFramePercentage says
    // (0.8 px at 800 px wide): a cursor parked at x=2 spun the camera for a
    // whole session past a 1 px floor. Over-detecting costs nothing, as the
    // guard also needs live rotation, the foreground and the world. Cursor
    // parks must stay clear of this (kDialogParkX in menus_monitors.cpp).
    constexpr int kMinBandPx = 6;
    s.bandPx = static_cast<int>(static_cast<float>(s.vpW) * pct);
    if (s.bandPx < kMinBandPx) s.bandPx = kMinBandPx;
    const int x = static_cast<int>(s.mouseX);
    const bool left  = s.vpW > 0 && x <= s.bandPx;
    const bool right = s.vpW > 0 && x >= static_cast<int>(s.vpW) - s.bandPx;
    s.edge = left ? "LEFT" : right ? "RIGHT" : "none";
    s.edgeNow = left || right;
}

// The fix: UpdateCamera's edge-turn (0x5f5e10) spins the camera while the
// cursor sits in the band. Only on a live spin (in-world, foreground, in the
// band, rotating), so a paused menu is never touched: the cursor is written
// straight to the viewport centre, as the post-load pipeline ignores
// injected motion.
bool ApplyEdgeGuard(const Sample& s) {
    if (!s.edgeNow || std::fabs(s.rateDegPerSec) < kGuardRateThresh ||
        !acc::hotkeys::IsForegroundGame() || !s.gui || s.vpW <= 0) {
        return false;
    }
    return SafeWriteOff<uint32_t>(s.gui, kGuiMouseXOffset, static_cast<uint32_t>(s.vpW / 2));
}

void TrackEpisode(DWORD now, const Sample& s, bool fired) {
    if (fired && !g_ep.active) {
        const int rightClick = SafeRead<int>(kAddrRightClickHeld, 0);
        bool mouseLook = false;
        acc::engine::GetMouseLook(mouseLook);
        g_ep.active      = true;
        g_ep.startMs     = now;
        g_ep.corrections = 0;
        g_ep.edgeFrames  = 0;
        g_ep.maxRate     = 0.0f;
        g_ep.startYaw    = s.quatYawDeg >= 0.0f ? s.quatYawDeg : 0.0f;
        acclog::Write("CameraSpinDiag",
            "edge-guard START: mouseX=%u/%d edge=%s rate=%.0f deg/s "
            "rightClick=%d mouseLook=%d",
            s.mouseX, static_cast<int>(s.vpW), s.edge, s.rateDegPerSec,
            rightClick, mouseLook ? 1 : 0);
    }
    if (!g_ep.active) return;
    if (fired) g_ep.corrections++;
    if (s.edgeNow) { g_ep.edgeFrames++; g_ep.lastEdgeMs = now; }
    if (std::fabs(s.rateDegPerSec) > g_ep.maxRate) g_ep.maxRate = std::fabs(s.rateDegPerSec);
    if (now - g_ep.lastEdgeMs >= kEpisodeQuietMs) {
        EndEpisode(now, s.quatYawDeg >= 0.0f ? s.quatYawDeg : g_ep.startYaw);
    }
}

// One line per full uncommanded turn, with every input UpdateCamera turns on.
void ReportSpinTurn(DWORD now, const Sample& s, int due) {
    const int rightClick = SafeRead<int>(kAddrRightClickHeld, 0);
    bool mouseLook = false;
    acc::engine::GetMouseLook(mouseLook);
    // K2-only witnesses of the camera's own turn path; -1 on K1 or without
    // a module.
    float acclMirror = -1.0f;
    int   turnedFlag = -1;
    if (acc::game::IsKotor2()) {
        void* mod = acc::engine::GetClientModule();
        acclMirror = SafeReadOff<float>(mod, kK2ModuleAcclMirrorOffset, -1.0f);
        turnedFlag = SafeReadOff<int>(mod, kK2ModuleTurnedFlagOffset, -1);
    }
    // The engine's held-turn-key view (a stranded flag shows here even when
    // the re-sync sees nothing) and K2's native right stick.
    float turnAxis = 0.0f;
    const bool haveTurnAxis = acc::engine::PollTurnAxis(&turnAxis);
    float stickX = 0.0f, stickY = 0.0f;
    const bool haveStick = acc::engine::ReadNativeStickAxes(&stickX, &stickY);
    char turnAxisBuf[16];
    char stickBuf[16];
    if (haveTurnAxis) _snprintf_s(turnAxisBuf, sizeof(turnAxisBuf), _TRUNCATE, "%.2f", turnAxis);
    else              _snprintf_s(turnAxisBuf, sizeof(turnAxisBuf), _TRUNCATE, "n/a");
    if (haveStick)    _snprintf_s(stickBuf, sizeof(stickBuf), _TRUNCATE, "%.2f", stickX);
    else              _snprintf_s(stickBuf, sizeof(stickBuf), _TRUNCATE, "n/a");
    acclog::Write("CameraSpinDiag",
        "UNCOMMANDED ROTATION x%d: quatTurn=%.0f deg posTurn=%.0f deg "
        "rate=%.0f deg/s duration=%ums frames=%d | mouseX=%u/%d "
        "mouseY=%u band=%dpx edge=%s mouseLook=%d rightClick=%d "
        "heldMoveVk=0x%02x camDist=%.1fm quatYaw=%.1f posYaw=%.1f "
        "| playerYaw=%.1f playerRate=%.0f deg/s playerTurn=%.0f deg "
        "turnAxis=%s stickX=%s acclMirror=%.2f turnedFlag=%d",
        due, g_spin.accumQuat, g_spin.accumPos, s.rateDegPerSec,
        now - g_spin.startMs, g_spin.frames, s.mouseX, static_cast<int>(s.vpW),
        s.mouseY, s.bandPx, s.edge, mouseLook ? 1 : 0, rightClick,
        acc::engine_keymap::FirstMovementKeyHeld(),
        s.camDistM, s.quatYawDeg, s.posYawDeg,
        s.playerYawDeg, s.playerRateDps, g_spin.accumPlayer,
        turnAxisBuf, stickBuf, acclMirror, turnedFlag);
    if (due >= kSpinMaxReports) {
        acclog::Write("CameraSpinDiag",
            "uncommanded-rotation: report cap reached (%d turns) — "
            "further turns this episode not logged", kSpinMaxReports);
    }
}

// The cause-agnostic spin detector: the camera turning while the player
// commands nothing leaves a record whatever drove it. Log-only on purpose:
// correcting an unknown cause blind risks fighting a legitimate camera.
// The K2 field report's cause (a turn-key release the engine discarded) is
// fixed by engine_keyboard_resync.cpp, so a spin from now on should end
// within a fraction of a second beside a "KeyboardResync: synthesised
// release" line; a longer one is another cause. "Commanding" covers keys,
// not a right-drag, hence the cursor and right-click in the report.
void WatchUncommandedRotation(DWORD now, const Sample& s, bool commanding, bool notOurs) {
    if (notOurs || commanding || !acc::hotkeys::IsForegroundGame()) {
        EndSpinWatch(now, commanding ? "player input" : "camera handed over");
        return;
    }
    // Either camera witness may be the live one, on the same threshold.
    const float absRate = std::fabs(s.rateDegPerSec);
    const bool rotating = (s.haveRate && absRate >= kSpinRateThresh) ||
                          (s.havePosStep && std::fabs(s.posRateDps) >= kSpinRateThresh);
    if (!rotating) {
        EndSpinWatch(now, "rotation stopped");
        return;
    }
    if (!g_spin.active) {
        g_spin.active  = true;
        g_spin.startMs = now;
    }
    ++g_spin.frames;
    if (s.haveRate)       g_spin.accumQuat   += s.quatStepDeg;
    if (s.havePosStep)    g_spin.accumPos    += s.posStepDeg;
    if (s.havePlayerStep) g_spin.accumPlayer += s.playerStepDeg;
    if (absRate > g_spin.maxRate) g_spin.maxRate = absRate;
    // No legitimate engine behaviour turns a player camera a whole
    // revolution on its own.
    float turns = std::fabs(g_spin.accumQuat);
    if (std::fabs(g_spin.accumPos) > turns) turns = std::fabs(g_spin.accumPos);
    const int due = static_cast<int>(turns / kSpinReportDeg);
    if (due > g_spin.reports && g_spin.reports < kSpinMaxReports) {
        g_spin.reports = due;
        ReportSpinTurn(now, s, due);
    }
}

// The quaternion and bearing yaws are only comparable in the ordinary follow
// pose (no cinematic, screen or snap-turn of ours, no turn input, a
// plausible distance), and a regression persists where a transient does not.
void CheckReadAnomaly(DWORD now, const Sample& s, bool commanding, bool notOurs) {
    const bool meaningful = s.quatYawDeg >= 0.0f && s.posYawDeg >= 0.0f &&
                            s.camDistM >= kFollowDistMinM && s.camDistM <= kFollowDistMaxM &&
                            !notOurs && !commanding;
    if (!meaningful) {
        g_anomalySinceMs = 0;
        return;
    }
    const float d = AngularDelta(s.quatYawDeg, s.posYawDeg);
    if (std::fabs(d) <= kReadAnomalyDeg) {
        if (g_anomalyOpen) {
            acclog::Write("CameraSpinDiag",
                "read-anomaly cleared: quatYaw=%.1f posYaw=%.1f diff=%.1f",
                s.quatYawDeg, s.posYawDeg, d);
            g_anomalyOpen = false;
        }
        g_anomalySinceMs = 0;
        return;
    }
    if (g_anomalySinceMs == 0) g_anomalySinceMs = now;
    const DWORD held = now - g_anomalySinceMs;
    if (held >= kAnomalyPersistMs &&
        (!g_anomalyOpen || now - g_lastAnomalyMs >= kAnomalyRepeatMs)) {
        acclog::Write("CameraSpinDiag",
            "READ ANOMALY (held %ums): quatYaw=%.1f posYaw=%.1f "
            "diff=%.1f camDist=%.1fm mouseX=%u/%d — the quaternion yaw "
            "read may have regressed; the edge guard's rotation test "
            "depends on it",
            held, s.quatYawDeg, s.posYawDeg, d, s.camDistM, s.mouseX, static_cast<int>(s.vpW));
        g_lastAnomalyMs = now;
        g_anomalyOpen   = true;
    }
}

}  // namespace

void Tick() {
    const DWORD now = acc::tick::NowMs();
    // Out of the world: drop the rate samples (no bogus jump on re-entry)
    // and close what is open (an area transition mid-spin).
    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) {
        g_state.have = false;
        g_state.havePos = false;
        g_state.havePlayer = false;
        g_anomalySinceMs = 0;
        EndSpinWatch(now, "left the world");
        if (g_ep.active) EndEpisode(now, g_ep.startYaw);
        return;
    }
    Sample s;
    SampleYaws(now, playerPos, s);
    SampleCursor(s);
    TrackEpisode(now, s, ApplyEdgeGuard(s));

    // Both scan (panel stack, key state): resolved once for both checks.
    const bool commanding = acc::engine_keymap::AnyMovementKeyHeld();
    const bool notOurs = CameraDrivenByNonPlayer();
    WatchUncommandedRotation(now, s, commanding, notOurs);
    CheckReadAnomaly(now, s, commanding, notOurs);
}

}  // namespace acc::camera_spin_guard
