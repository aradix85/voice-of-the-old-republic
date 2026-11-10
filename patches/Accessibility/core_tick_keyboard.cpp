// Keeping the engine's DirectInput keyboard alive: the cold-start reacquire retry and the
// mid-session dead-keyboard watchdog.

#include "core_tick_internal.h"

#include "audio_footstep_suppress.h"
#include "bringup_announce.h"
#include "camera_announce.h"
#include "camera_orient.h"
#include "engine_keymap.h"  // FirstMovementKeyHeld — dead-keyboard watchdog probe
#include "focus_guard.h"
#include "engine_player.h"
#include "log.h"
#include "engine_input.h"

namespace acc::tick {

namespace {

// ---------------------------------------------------------------------------
// Cold-start DirectInput reacquire retry
// ---------------------------------------------------------------------------
// On a fresh launch the engine can reach the main menu with its DirectInput
// keyboard unacquired, so menu input is dead until the user alt-tabs (which
// forces a SetActive(0)->(1) edge). menus.cpp fires one ForceReacquireInput at
// MainMenu first-sight, but that single shot is not enough on every machine:
// in tester kenny's en-US 0.5.1 log the edge fired at first-sight yet the
// keyboard stayed dead for ~29 s — the mouse worked the whole time (cursor
// events flowed) but no keypress reached a panel until he physically clicked
// into the window. The likeliest cause is the engine recreating its Render
// Window shortly after first-sight and silently dropping the acquire we just
// drove; with no retry, nothing re-establishes it.
//
// This re-drives the edge every tick until input is provably live, then never
// runs again for the session. Guards:
//   * Stops permanently once acc::bringup_announce::IsInputPumpLive() — the
//     pump has delivered an event to a panel. Immune to the mislabeled
//     cursor-position channel that floods the input hook (it doesn't drive
//     SetActiveControl, so it never trips the latch).
//   * Only while the game owns the foreground — never acquire under another
//     app, so input keeps mirroring foreground (no nav-key bleed into the
//     background game while a screen reader / other window is in front).
//   * Never while a key is held (2026-08-14). The original text here claimed
//     the kReacquireRetryMs throttle kept "a key that is mid-delivery" from
//     being shredded by a (0) phase. That was wrong, and the decompile in the
//     watchdog block below says why: the 0->1 leg calls ClearKeyboardBuffer,
//     which drains EVERY queued transition, and the engine reads keys buffered
//     rather than as level state — so a discarded release is discarded for
//     good and the engine holds that key down forever. A throttle cannot help
//     with that; only refusing to cycle while something is held can. Cheap
//     here too: the tester scenario this path exists for (kenny, 0.5.1) is
//     someone pressing keys into a dead main menu, so there are gaps between
//     presses to retry in, and skipping a beat costs one 200 ms tick.
//   * Bounded to kReacquireMaxAttempts so a genuinely stuck keyboard (e.g.
//     another process holding it exclusive) doesn't cycle forever — we log a
//     give-up line and fall silent.
constexpr ULONGLONG kReacquireRetryMs    = 200;
constexpr int       kReacquireMaxAttempts = 50;  // ~10 s at 200 ms cadence
constexpr ULONGLONG kReacquireSettleMs   = 150;  // quiet after a release

// ---------------------------------------------------------------------------
// Dead-keyboard watchdog (mid-session)
// ---------------------------------------------------------------------------
// RetryColdStartReacquire above latches permanently once the pump goes live, so
// it only ever protects the launch window. The same failure recurs mid-session:
// an alt-tab out and back makes the engine recreate its Render Window, the
// focus-gain path fires ONE SetActive(0)->(1) cycle, and when that single shot
// doesn't take the engine's keyboard stays dead for the rest of the session.
//
// Witnessed in patch-20260809-090145.log: window recreated at 09:03:23, the
// focus-gain reacquire fired at 09:03:26, and then ZERO key presses reached the
// engine (no Diag.ClientHIE val=128, no Menus.Input) until a second alt-tab at
// 09:03:59. The engine's quit-confirm popup was up, correctly announced, with
// OK and Cancel focusable — and unreachable, because no keystroke could get
// to it. Cold start needed 13 retries that same session; one shot was never
// going to be enough.
//
// Detection is the asymmetry itself: mod hotkeys are polled through
// GetAsyncKeyState and keep working when the engine's DirectInput is dead. So
// "an ENGINE-BOUND key is held according to Win32, while the engine has not
// seen a key press for a good while" means the keyboard died, and nothing else
// produces that combination. Keys the engine does not bind are excluded — the
// mod's own cycle keys (`,` `.` `-`) legitimately never reach the engine, and
// treating those as evidence would fire the recovery constantly.
//
// ---------------------------------------------------------------------------
// 2026-08-14 — that reasoning was wrong twice over, and the pair of them made
// this watchdog the cause of the runaway-camera / walks-off-on-its-own reports
// (niki + Lenny beta logs, both 0.7.2). Recorded in full because the shape of
// the mistake matters more than the patch.
//
// (1) "the engine has not seen a key press" was never measurable. The liveness
//     stamp (NoteEngineKeyEvent) is written only from the GUI-routed hooks, on
//     val==128. In-world keyboard never passes through them — it is polled
//     straight off DirectInput (see gui-and-input-internals.md: BufferEvent is
//     mouse-only, "keyboard events take a different ingest path"). niki's whole
//     session stamped 39 times, every one a panel logical code. So during
//     ordinary world play "engine silent" is PERMANENTLY true; it climbed past
//     70 s in Lenny's log while he was visibly playing. The only thing left
//     gating the watchdog was "a probe key held >= 1.5 s" — which is the
//     signature of walking, not of a dead keyboard.
//
// (2) The recovery is not passive. ForceReacquireInput drives
//     CExoInput::SetActive 0->1, and the chain underneath (decompiled
//     2026-08-14: CExoInputInternal::SetActive -> CExoRawInputInternal::
//     SetActive) does two things that matter here. The 1->0 leg Unacquire()s
//     the keyboard, mouse and every joystick, so nothing is delivered at all
//     while it is down. The 0->1 leg Acquire()s and then calls
//     ClearKeyboardBuffer + ClearMouseBuffer, which drain the DirectInput
//     buffer with GetDeviceData until empty and zero the per-device event
//     state. And the engine reads keys BUFFERED (GetKeyboardBuffer ->
//     GetDeviceData), i.e. as a stream of transitions, not as polled level
//     state. So a discarded key-up is discarded permanently — nothing ever
//     re-derives it, and the engine's idea of "this key is down" stays stuck
//     down forever.
//
// Together: hold a movement key for 1.5 s, and this fired every 700 ms for as
// long as the hold lasted (25 consecutive cycles in niki's log, 256 firings in
// Lenny's), each one deleting whatever transitions were pending. Sooner or
// later the deleted one is the release. Lenny's log has the whole chain in
// eight lines at 21:01 — camera_orient arms and holds A; three recovery cycles
// fire, one of them reporting vk=0x41 (our own synthesised A); the rotation
// times out having moved 0 degrees because input was dead; it sends A up into
// a cleared buffer; and from 21:01:07 the camera turns left through every
// sector at ~200 deg/s for fifteen seconds with nothing armed. niki's log has
// the walking twin: yaw frozen at 164.0 deg for 39 s while the player slides
// dead straight, x pinned at 127.9, y climbing 213 -> 251.
//
// The rewrite below keeps the original intent — a genuinely dead keyboard
// still wakes itself — under three rules:
//
//   * NEVER cycle while a key is down. This is the load-bearing one, and it
//     holds regardless of how good the detector is: a buffer clear with
//     nothing held cannot strand a release, because there is no release to
//     strand. A hold that looks unanswered now only ARMS a suspicion; the
//     recovery waits for the release.
//   * Liveness has a second channel that does cover the in-world path. If the
//     player is moving or the camera is turning, the engine is acting on the
//     keyboard whether or not any GUI event was stamped. False "alive" is
//     safe here; false "dead" is what did the damage.
//   * Bounded, like the cold-start path already is. s_cycles was counted and
//     logged but never checked against anything.
//
// Also excluded: our own synthesised holds. camera_orient drives the engine by
// holding a real scancode down, so it tripped the probe (vk=0x41 / 0x44 in
// both logs) and then had its own key-up eaten by the cycle it had triggered.
constexpr ULONGLONG kDeadKeyboardMs   = 1500;  // silence that counts as dead
constexpr ULONGLONG kDeadRecoveryMs   = 700;   // between recovery cycles
constexpr ULONGLONG kDeadSettleMs     = 250;   // quiet after release before a cycle
constexpr int       kDeadMaxCycles    = 6;     // per episode, then give up

// World-response liveness thresholds. Deliberately coarse: this only has to
// separate "the engine is acting on input" from "nothing is happening at all",
// and anything that moves the player or the camera counts, keyboard-driven or
// not. Both are per-tick deltas, so the bar is low.
constexpr float kWorldMoveEpsM   = 0.05f;
constexpr float kWorldTurnEpsDeg = 1.0f;

// Set whenever the world visibly responded. The second liveness channel — see
// point (1) above for why LastEngineKeyEventMs alone cannot see world input.
ULONGLONG g_lastWorldResponseMs = 0;

// ---------------------------------------------------------------------------
// Held-key probe — shared by both reacquire paths below
// ---------------------------------------------------------------------------
// Engine-bound keys a player actually presses when nothing is responding. All
// are real engine binds (GUI confirm/cancel/nav, leader cycle, movement).
//
// Note what this is and is not. It answers "is the player leaning on a key the
// engine ought to be acting on" — nothing more. It was once read as half of a
// dead-keyboard proof, on the assumption that a healthy engine answers within a
// frame and so the watchdog would never arm; the arrow keys here are exactly
// how both beta testers walk, and holding one for 1.5 s armed it every time.
// The proof now lives in the liveness channels, not in this list.
//
// Its load-bearing use is the other one: BOTH reacquire paths refuse to drive a
// SetActive edge while this returns non-zero, because the 0->1 leg clears the
// DirectInput keyboard buffer and the transition it would delete is the held
// key's release. See the watchdog's comment block for the decompile.
int HeldEngineBoundKey() {
    static const int kProbe[] = {
        VK_RETURN, VK_ESCAPE, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_TAB, VK_SPACE
    };
    for (int vk : kProbe) {
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    return acc::engine_keymap::FirstMovementKeyHeld();
}

void SampleWorldResponse(ULONGLONG now) {
    static bool   s_have   = false;
    static Vector s_pos    = {0.0f, 0.0f, 0.0f};
    static float  s_yawDeg = 0.0f;

    Vector pos = {0.0f, 0.0f, 0.0f};
    float  yawDeg  = 0.0f;
    const bool havePos = acc::engine::GetPlayerPosition(pos);
    const bool haveYaw =
        acc::camera_announce::TryGetCameraEngineYawDegrees(yawDeg);

    // Out of the world entirely (menus, load screens): no sample to compare
    // against, and the GUI stamp covers those contexts anyway.
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

    // Nothing-held precondition, plus a settle so the release itself has
    // drained out of the buffer before we clear it.
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

    // Never acquire under another app — same rule as the cold-start retry.
    if (!acc::focus_guard::GameOwnsForeground()) {
        s_heldSince = s_suspectSince = s_releasedAt = 0;
        return;
    }

    const ULONGLONG now = GetTickCount64();
    SampleWorldResponse(now);

    // Our own synthesised hold proves nothing about the player's keyboard, and
    // cycling over it is what strands the rotation key. camera_orient releases
    // within kTimeoutMs, so this defers rather than disables.
    if (acc::camera_orient::IsActive()) {
        s_heldSince = s_suspectSince = s_releasedAt = 0;
        return;
    }

    // Any channel counts as proof the engine is hearing input:
    //   * the GUI stamp, for panel contexts;
    //   * the world response, for ordinary in-world play;
    //   * the leader's footsteps, for the case the other two miss — walking
    //     into a wall. Position and camera yaw both sit still there, but the
    //     engine is plainly acting on the key, and says so by playing a
    //     footstep per step. Added 2026-08-14 off the first post-fix K2 log,
    //     where a dead end ("dead end") armed the watchdog three times in
    //     forty seconds while FootstepSup logged steps the whole way through.
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
        // Whatever we concluded, do not cycle: SetActive(1) clears the
        // keyboard buffer, and with a key down the transition it deletes is
        // that key's release.
        return;
    }

    // --- everything released from here on ---
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
    // Let the release itself drain before clearing the buffer, and don't eat
    // the player's next press if they are typing through the dead spell.
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
