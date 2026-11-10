// Enter in view mode: the deferred dispatch to the hovered object, or a walk to the cursor.

#include "view_mode_internal.h"

#include "engine_player.h"
#include "guidance_autowalk.h"
#include "hotkeys.h"
#include "interact_dispatch.h"
#include "log.h"
#include "narrated_target.h"
#include "strings.h"
#include "speech.h"

namespace acc::view_mode {

namespace {

// Single-tick Enter ownership. Set by PollEnter, read-and-cleared by
// interact_hotkey::PollHotkey so the same press can't dispatch twice.
bool g_enter_consumed_this_tick = false;

// Deferred-dispatch state. WalkTo silently no-ops when input has been at 0
// for many ticks — engine needs a fresh 1→0 transition right before AI
// dispatch. Synchronous round-trip in the same tick doesn't work either:
// engine never processes the intermediate enabled=1. So PollEnter exits
// view mode + re-enables input, arms this struct, and Tick fires the
// dispatch on the next tick after the engine has settled.
//
// Hover obj/handle are NOT snapshotted: hover speech stamps the unified
// narrated_target slot, ProcessPendingDispatch reads from there at
// dispatch time. Only the empty-cursor WalkTo path keeps a position
// snapshot (no associated object).
struct PendingDispatch {
    bool     active        = false;
    bool     hasHover      = false;
    Vector   cursor_pos    = {0.0f, 0.0f, 0.0f};
    bool     forceRadial   = false;
    DWORD    armed_at_ms   = 0;
};
PendingDispatch g_pending;

constexpr DWORD kPendingDispatchMinElapsedMs = 16;  // ~1 frame at 60fps

}  // namespace

// Enter / Shift+Enter dispatch while view mode is active.
//   hover target present → DispatchInteract(obj, handle, forceRadial)
//   no hover target      → speak pre-roll + WalkTo(cursor_pos)
// View mode exits synchronously before dispatch (autowalk needs an
// unfrozen character). Actual dispatch is deferred via g_pending — see
// PendingDispatch comment for the engine-state reason.
void PollEnter() {
    bool risingPlain  = acc::hotkeys::Pressed(
                            acc::hotkeys::Action::InteractTarget);
    bool risingForce  = acc::hotkeys::Pressed(
                            acc::hotkeys::Action::InteractForceRadial);
    if (!risingPlain && !risingForce) return;

    // Claim this tick's Enter before exiting view mode; interact_hotkey
    // runs later in the same OnUpdate and reads-and-clears this flag.
    g_enter_consumed_this_tick = true;

    bool forceRadial = risingForce;
    const char* keyTag = forceRadial ? "Shift+Enter" : "Enter";

    // Snapshot only view-mode-local state. Hover obj/handle live in the
    // unified narrated_target slot — ProcessPendingDispatch reads them
    // from there at dispatch time.
    Vector cursor_pos = g_state.cursor_pos;
    bool   hasHover   = g_state.hover_pending_obj != nullptr &&
                        g_state.hover_pending != 0;

    g_state.active = false;
    bool restored = acc::engine::SetPlayerInputEnabled(true);
    acclog::Write("ViewMode", "EXIT (deferred dispatch) input_restored=%d hasHover=%d",
        restored ? 1 : 0, hasHover ? 1 : 0);

    g_pending.active        = true;
    g_pending.hasHover      = hasHover;
    g_pending.cursor_pos    = cursor_pos;
    g_pending.forceRadial   = forceRadial;
    g_pending.armed_at_ms   = GetTickCount();

    acclog::Write("ViewMode", "%s -> dispatch armed for next tick "
        "(hasHover=%d cursor=(%.2f,%.2f,%.2f) forceRadial=%d)",
        keyTag, hasHover ? 1 : 0,
        cursor_pos.x, cursor_pos.y, cursor_pos.z,
        forceRadial ? 1 : 0);
}

// Runs at the top of Tick BEFORE the IsActive() gate — by design, this
// fires after PollEnter exited view mode synchronously.
void ProcessPendingDispatch() {
    if (!g_pending.active) return;

    DWORD elapsed = GetTickCount() - g_pending.armed_at_ms;
    if (elapsed < kPendingDispatchMinElapsedMs) return;

    // Snapshot and clear before dispatch — the dispatch path may re-enter
    // our hooks; re-entry must not see g_pending as active.
    bool     hasHover    = g_pending.hasHover;
    Vector   cursor_pos  = g_pending.cursor_pos;
    bool     forceRadial = g_pending.forceRadial;
    g_pending.active = false;

    const char* keyTag = forceRadial ? "Shift+Enter" : "Enter";

    if (hasHover) {
        // Read the hover target live: a passive_narrate stamp between
        // PollEnter and now wins — "last narrated wins". Stale slot (rare,
        // object destroyed within the one-tick gap) speaks GuidanceNoFocus
        // instead of dispatching against a zombie pointer.
        acc::narrated_target::Slot slot;
        if (!acc::narrated_target::TryGet(slot)) {
            const char* msg = acc::strings::Get(
                acc::strings::Id::GuidanceNoFocus);
            acc::speech::Speak(msg, /*interrupt=*/true);
            acclog::Write("ViewMode", "%s deferred -> [%s] (hover stale at dispatch) "
                "elapsed=%lums",
                keyTag, msg, static_cast<unsigned long>(elapsed));
            return;
        }
        acclog::Write("ViewMode", "%s deferred -> DispatchInteract obj=%p handle=0x%08x "
            "elapsed=%lums (narrated target)",
            keyTag, slot.obj, slot.handle,
            static_cast<unsigned long>(elapsed));
        acc::interact::DispatchInteract(slot.obj, slot.handle, forceRadial);
        return;
    }

    // Empty cursor — pre-roll then ForceWalkTo (queue-bypass). Plain
    // WalkTo's queue path stays asleep after view mode's sustained
    // SetEnabled(false); ForceMoveToPoint dispatches directly.
    const char* preroll = acc::strings::Get(acc::strings::Id::GuidingToPoint);
    acc::speech::Speak(preroll, /*interrupt=*/true);

    bool ok = acc::guidance::ForceWalkTo(cursor_pos);
    acclog::Write("ViewMode", "%s deferred -> ForceWalkTo cursor=(%.2f,%.2f,%.2f) ok=%d "
        "elapsed=%lums (no hover target)",
        keyTag, cursor_pos.x, cursor_pos.y, cursor_pos.z, ok ? 1 : 0,
        static_cast<unsigned long>(elapsed));
}

bool ConsumedEnterThisTick() {
    bool was = g_enter_consumed_this_tick;
    g_enter_consumed_this_tick = false;
    return was;
}

}  // namespace acc::view_mode
