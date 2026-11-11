// Keeping the engine's DirectInput keyboard alive: the cold-start reacquire
// retry and the mid-session dead-keyboard watchdog.
//
// Both recover by driving CExoInput::SetActive 0 -> 1 (ForceReacquireInput).
// The 1 -> 0 leg Unacquire()s the keyboard, mouse and joysticks; the 0 -> 1
// leg Acquire()s and then drains the DirectInput buffers (ClearKeyboardBuffer,
// ClearMouseBuffer). The engine reads keys buffered, as a stream of
// transitions rather than as level state, so a key-up drained that way is
// lost for good and the engine holds that key down forever. Hence the rule
// both paths keep: never cycle while a key is held, and let a release settle
// first.

#include <cmath>

#include "core_tick_internal.h"
#include "audio_footstep_suppress.h"
#include "bringup_announce.h"
#include "camera_announce.h"
#include "camera_orient.h"
#include "engine_keymap.h"
#include "focus_guard.h"
#include "hotkeys.h"
#include "engine_player.h"
#include "log.h"
#include "engine_input.h"

namespace acc::tick {

namespace {

// The engine-bound key held right now, or 0: GUI confirm, cancel and nav,
// leader cycle, movement. This says only that the player leans on a key the
// engine ought to act on; it proves nothing about a dead keyboard (holding
// an arrow key is how players walk). Its use is the hold rule above.
int HeldEngineBoundKey() {
    static const int kProbe[] = {
        VK_RETURN, VK_ESCAPE, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_TAB, VK_SPACE
    };
    for (int vk : kProbe) {
        if (acc::hotkeys::KeyDown(vk)) return vk;
    }
    return acc::engine_keymap::FirstMovementKeyHeld();
}

// On a fresh launch the engine can reach the main menu with its keyboard
// unacquired, so menu input is dead until the user alt-tabs. menus_focus.cpp fires
// one ForceReacquireInput at MainMenu first sight, but the engine can
// recreate its render window moments later and drop that acquire. So the
// edge is re-driven every kReacquireRetryMs until the input pump has
// delivered an event to a panel (IsInputPumpLive), then never again: only
// while the game owns the foreground, never while a key is held, and at most
// kReacquireMaxAttempts times (another process may hold the keyboard
// exclusive).
constexpr ULONGLONG kReacquireRetryMs     = 200;
constexpr int       kReacquireMaxAttempts = 50;
constexpr ULONGLONG kReacquireSettleMs    = 150;  // quiet after a release

// The mid-session twin. An alt-tab out and back recreates the render window,
// the focus-gain path fires one SetActive cycle, and when that doesn't take
// the keyboard stays dead. Mod hotkeys keep working through
// GetAsyncKeyState, so the fingerprint is an engine-bound key held for
// kDeadKeyboardMs while the engine shows no sign of hearing input. Those
// signs are the GUI key stamp (LastEngineKeyEventMs, panel contexts only:
// in-world keys are polled straight off DirectInput and never stamped), the
// world visibly responding (the player moved or the camera turned), and the
// leader's footsteps (walking into a wall moves nothing but still steps). A
// hold that looks unanswered only arms a suspicion; the recovery runs after
// the release, at most kDeadMaxCycles times per episode.
constexpr ULONGLONG kDeadKeyboardMs = 1500;  // silence that counts as dead
constexpr ULONGLONG kDeadRecoveryMs = 700;   // between recovery cycles
constexpr ULONGLONG kDeadSettleMs   = 250;   // quiet after release before a cycle
constexpr int       kDeadMaxCycles  = 6;     // per episode, then give up

// Per-tick deltas that count as the world responding, keyboard-driven or
// not. Deliberately coarse: they only separate "acting on input" from
// "nothing happening at all".
constexpr float kWorldMoveEpsM   = 0.05f;
constexpr float kWorldTurnEpsDeg = 1.0f;

ULONGLONG g_lastWorldResponseMs = 0;

void SampleWorldResponse(ULONGLONG now) {
    static bool   s_have   = false;
    static Vector s_pos    = {0.0f, 0.0f, 0.0f};
    static float  s_yawDeg = 0.0f;

    Vector pos = {0.0f, 0.0f, 0.0f};
    float  yawDeg  = 0.0f;
    const bool havePos = acc::engine::GetPlayerPosition(pos);
    const bool haveYaw =
        acc::camera_announce::TryGetCameraEngineYawDegrees(yawDeg);

    // Out of the world (menus, load screens): nothing to compare, and the GUI
    // stamp covers those contexts.
    if (!havePos && !haveYaw) {
        s_have = false;
        return;
    }

    if (s_have) {
        bool responded = false;
        if (havePos) {
            responded = DistXYSq(pos, s_pos) >=
                        (kWorldMoveEpsM * kWorldMoveEpsM);
        }
        if (!responded && haveYaw) {
            // Shortest signed delta, so the 360->0 wrap isn't read as a jump.
            float d = std::fmod(yawDeg - s_yawDeg + 540.0f, 360.0f) - 180.0f;
            responded = std::fabs(d) >= kWorldTurnEpsDeg;
        }
        if (responded) g_lastWorldResponseMs = now;
    }

    s_pos    = pos;
    s_yawDeg = yawDeg;
    s_have   = true;
}

}  // namespace

void RetryColdStartReacquire() {
    static bool      s_done       = false;
    static int       s_attempts   = 0;
    static ULONGLONG s_lastMs     = 0;
    static ULONGLONG s_releasedAt = 0;
    static bool      s_wasHeld    = false;

    if (s_done) return;

    if (acc::bringup_announce::IsInputPumpLive()) {
        if (s_attempts > 0) {
            acclog::Write("EngineInput",
                "cold-start reacquire: input pump live after %d retr%s — done",
                s_attempts, s_attempts == 1 ? "y" : "ies");
        }
        s_done = true;
        return;
    }

    if (!acc::focus_guard::GameOwnsForeground()) return;

    ULONGLONG nowMs = GetTickCount64();

    if (HeldEngineBoundKey() != 0) {
        s_wasHeld = true;
        return;
    }
    if (s_wasHeld) {
        s_wasHeld    = false;
        s_releasedAt = nowMs;
    }
    if (s_releasedAt != 0 && nowMs - s_releasedAt < kReacquireSettleMs) return;

    if (s_lastMs != 0 && nowMs - s_lastMs < kReacquireRetryMs) return;
    s_lastMs = nowMs;

    if (s_attempts >= kReacquireMaxAttempts) {
        acclog::Write("EngineInput",
            "cold-start reacquire: gave up after %d attempts — keyboard still "
            "not live while foreground (another app may hold it exclusive)",
            s_attempts);
        s_done = true;
        return;
    }

    ++s_attempts;
    acc::engine::ForceReacquireInput();
}

void WatchDeadKeyboard() {
    static ULONGLONG s_heldSince    = 0;  // this hold began
    static ULONGLONG s_suspectSince = 0;  // ...and went unanswered from here
    static ULONGLONG s_releasedAt   = 0;  // everything let go at
    static ULONGLONG s_lastCycle    = 0;
    static int       s_cycles       = 0;
    static bool      s_gaveUp       = false;

    if (!acc::focus_guard::GameOwnsForeground()) {
        s_heldSince = s_suspectSince = s_releasedAt = 0;
        return;
    }

    const ULONGLONG now = GetTickCount64();
    SampleWorldResponse(now);

    // camera_orient turns the camera by holding a real scancode down. That
    // proves nothing about the player's keyboard, and a cycle over it would
    // strand its key-up. It releases within its timeout, so this only defers.
    if (acc::camera_orient::IsActive()) {
        s_heldSince = s_suspectSince = s_releasedAt = 0;
        return;
    }

    const ULONGLONG lastKey  = acc::engine::LastEngineKeyEventMs();
    const ULONGLONG lastStep =
        acc::audio::footstep_suppress::LastLeaderFootstepMs();
    ULONGLONG lastAlive = lastKey;
    if (g_lastWorldResponseMs > lastAlive) lastAlive = g_lastWorldResponseMs;
    if (lastStep > lastAlive)              lastAlive = lastStep;
    const bool alive = lastAlive != 0 && now - lastAlive < kDeadKeyboardMs;

    const int heldVk = HeldEngineBoundKey();
    if (heldVk != 0) {
        if (s_heldSince == 0) {
            s_heldSince    = now;
            s_suspectSince = 0;   // a new hold is innocent until unanswered
        }
        s_releasedAt = 0;
        if (alive) {
            s_suspectSince = 0;   // this hold IS being acted on
            s_cycles       = 0;
            s_gaveUp       = false;
        } else if (s_suspectSince == 0 &&
                   now - s_heldSince >= kDeadKeyboardMs) {
            s_suspectSince = now;
            acclog::Write("EngineInput",
                "dead-keyboard watchdog: vk=0x%02x held %llums unanswered "
                "(engine silent %llums) — armed, waiting for release",
                heldVk, now - s_heldSince,
                lastAlive ? now - lastAlive : 0ull);
        }
        return;
    }

    // Everything released from here on.
    if (s_heldSince != 0) {
        s_heldSince  = 0;
        s_releasedAt = now;
    }
    if (s_suspectSince == 0) {
        s_cycles = 0;
        return;
    }
    if (alive) {                       // it woke up on its own
        s_suspectSince = 0;
        s_cycles       = 0;
        s_gaveUp       = false;
        return;
    }
    // Let the release drain before clearing the buffer, and don't eat the
    // player's next press while they type through the dead spell.
    if (s_releasedAt == 0 || now - s_releasedAt < kDeadSettleMs) return;
    if (s_lastCycle != 0 && now - s_lastCycle < kDeadRecoveryMs) return;

    if (s_cycles >= kDeadMaxCycles) {
        if (!s_gaveUp) {
            acclog::Write("EngineInput",
                "dead-keyboard watchdog: gave up after %d cycles — keyboard "
                "still not answering (another app may hold it exclusive)",
                s_cycles);
            s_gaveUp = true;
        }
        return;
    }

    s_lastCycle = now;
    ++s_cycles;
    acclog::Write("EngineInput",
        "dead-keyboard watchdog: hold went unanswered for %llums, all keys "
        "released — driving recovery cycle #%d",
        now - s_suspectSince, s_cycles);
    acc::engine::ForceReacquireInput();
}

}  // namespace acc::tick
