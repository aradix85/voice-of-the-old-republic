// Enter in view mode: the deferred dispatch to the hovered object, or a walk
// to the cursor.

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

// This tick's Enter belongs to view mode: set by PollEnter, read and cleared
// by input_poll_router, so one press cannot dispatch twice.
bool g_enter_consumed_this_tick = false;

// The dispatch waits a tick: after input sat disabled for long, the engine
// ignores AI actions until it has seen input enabled, and a round trip
// within one tick is not seen. So PollEnter exits view mode and re-enables
// input, and Tick dispatches on the next tick. The hovered object is read
// from narrated_target at that point (the last narrated wins); only the
// cursor position is kept, for the walk.
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

    const bool forceRadial = risingForce;
    const char* keyTag = forceRadial ? "Shift+Enter" : "Enter";
    const Vector cursor_pos = g_state.cursor_pos;
    const bool   hasHover   = g_state.hover_pending_obj != nullptr &&
                              g_state.hover_pending != 0;

    g_state.active = false;
    bool restored = acc::engine::SetPlayerInputEnabled(true);
    acclog::Write("ViewMode", "EXIT (deferred dispatch) input_restored=%d hasHover=%d",
        restored ? 1 : 0, hasHover ? 1 : 0);

    g_pending = PendingDispatch{true, hasHover, cursor_pos, forceRadial, GetTickCount()};

    acclog::Write("ViewMode", "%s -> dispatch armed for next tick "
        "(hasHover=%d cursor=(%.2f,%.2f,%.2f) forceRadial=%d)",
        keyTag, hasHover ? 1 : 0,
        cursor_pos.x, cursor_pos.y, cursor_pos.z,
        forceRadial ? 1 : 0);
}

void ProcessPendingDispatch() {
    if (!g_pending.active) return;

    DWORD elapsed = GetTickCount() - g_pending.armed_at_ms;
    if (elapsed < kPendingDispatchMinElapsedMs) return;

    // Cleared before the dispatch, which may re-enter our hooks.
    const PendingDispatch p = g_pending;
    g_pending.active = false;
    const bool   hasHover    = p.hasHover;
    const Vector cursor_pos  = p.cursor_pos;
    const bool   forceRadial = p.forceRadial;

    const char* keyTag = forceRadial ? "Shift+Enter" : "Enter";

    if (hasHover) {
        // A slot gone stale in the one-tick gap (the object destroyed) says
        // so instead of dispatching on a dead pointer.
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

    // Nothing hovered: walk to the cursor. ForceWalkTo, since the plain
    // WalkTo queue stays asleep after view mode's long input freeze.
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
