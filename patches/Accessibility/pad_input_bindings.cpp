// Gamepad bindings both games share: routing a press into the mod's own
// overlays, the unified action menu, the trigger + face-button chord grid,
// the D-Pad in the world and on the map, and the shoulder chords.

#include "pad_input_internal.h"

#include "combat_queue.h"
#include "cycle_input.h"
#include "engine_input.h"
#include "engine_panels.h"
#include "examine_view.h"
#include "help.h"
#include "hotkeys.h"         // PadPress — reuse the keyboard actions' pollers
#include "interact_dispatch.h"
#include "log.h"
#include "narrated_target.h" // TryGet — is there a target to open the menu on?
#include "pad_quickmenu.h"   // the KOTOR 1 Y menu (K2 uses the engine's own)
#include "unified_action_menu.h"

namespace acc::pad {

namespace {

// ---------------------------------------------------------------------------
// The face-button chord grid
// ---------------------------------------------------------------------------
// A trigger held turns each of the four face buttons into a second binding.
// That is eight slots, and they are what closed the last of the gaps between
// what a keyboard player can reach and what a pad player can:
//
//   LT + A   announce the focused object (the minus key)
//   LT + B   open the action queue       (Shift+H)
//   LT + X   your own status             (H)
//   LT + Y   cancel combat               (F)
//   RT + A   look-around mode            (B)
//   RT + B   copy the last spoken text   (Ctrl+R)
//   RT + X   — free —
//   RT + Y   the level-up screen         (Shift+L)
//
// The grid is the point: "hold a trigger and press a face button" is one thing
// to remember instead of eight.
//
// What decides which half a binding lands in is ERGONOMICS, not category. The
// left trigger is the comfortable side — left index finger, right thumb, two
// hands working independently — so the bindings reached for often live there.
// The right trigger is the same-hand stretch: index tensed on the trigger while
// the thumb travels, which is fine for A and increasingly awkward toward X and
// Y. So the right half carries only what is rare and unhurried, and RT + X is
// deliberately left empty rather than filled with something that would then be
// hard to press.
//
// The queue's clear-all used to sit at LT + A. It is a bare Y inside the queue
// overlay now (ClaimQueueClearAll above), which freed the grid's single best
// slot for the readout a player uses most.
//
// Read from XInput on BOTH games rather than from the engine's own button
// events, because a chord has to be decided from the TRIGGER STATE and only
// XInput reports that (the engine never reads the trigger axis at all) — and
// because these have to answer on every screen, including the ones a given
// button's engine event does not reach. On KOTOR 2 the matching engine event is
// consumed while a trigger is held so the button's bare job does not also run.
//
// Every binding is routed as a synthetic press of the keyboard action it
// mirrors, never as a call into a handler: the world gate, the combat gate and
// the announcement each stay in the one place they already live.
//
// Returns true when a chord fired, which is also the caller's signal that the
// button's BARE job must not run. Marks the trigger chorded so its own solo job
// — the action menu on LT, the degrees announcement on RT — does not also fire
// when the player lets go.
// Cancel Combat is the engine's own [Keymapping] action, not a mod hotkey, so
// it is fired the way the quick menu fires its entries: synthesise whatever key
// the player has bound to it. Row 40 + 200; the ini's Action240 carries
// InputIndex 56, which is the letter F — the default this falls back to.
constexpr int kActCancelCombat = 240;
constexpr int kDikF            = 0x21;

// ---------------------------------------------------------------------------
// The D-Pad, and what it is for
// ---------------------------------------------------------------------------
// The D-Pad is not free in the world — it drives the engine's own
// CSWGuiActionMenuIos, whose level 1 (category) is D-Pad left/right and whose
// level 2 (entry) is D-Pad up/down, with A confirming. The earlier reading
// that it was inert was a blind test round watching a silent, purely visual
// highlight move; every round since has consumed the codes outright, which is
// why Pad.ActionMenu never logged a line.
//
// So the four codes are ours, and in the world they are the object cycle:
// left/right steps the object, up/down the category. That is the D-Pad's ONE
// job. It used to have two, switched by the left trigger — cycle and a
// permanently-open "live" action menu — and the second one ate the A button:
// with the mode on, A always belonged to the menu, so interacting with
// anything meant switching the mode back first. The action menu is a menu the
// player OPENS now (the left trigger, below), and while it is up it is an
// overlay, so RouteToOverlay claims the D-Pad before this function is reached.

// Open the unified action menu on whatever is focused — the pad's Shift+Enter.
// Returns true when it is up afterwards.
//
// With no focused target the target categories are empty, so fall through to
// the personal block — medpacs and stims are exactly what a player wants when
// nothing is targeted, and refusing to open would be the wrong answer.
bool OpenActionMenu() {
    acc::narrated_target::Slot slot{};
    const bool haveTarget = acc::narrated_target::TryGet(slot) && !slot.isMapPin;
    if (haveTarget) {
        // The Shift+Enter gesture: target categories first, and Left still
        // crosses into the personal block from there.
        acc::interact::InteractNarratedTarget(/*forceRadial=*/true);
    }
    if (!acc::unified_menu::IsActive()) {
        // No populated target row (a corpse, a non-combat NPC), or no target
        // at all. The personal block is still worth opening — wherever it has
        // something, which is NOT necessarily column 0: on a droid that column
        // is Force Powers and is always empty.
        acc::unified_menu::OpenAnyPersonal();
    }
    const bool up = acc::unified_menu::IsActive();
    acclog::Write("Pad", "action menu open target=%d up=%d",
                  haveTarget ? 1 : 0, up ? 1 : 0);
    return up;
}

}  // namespace

// Route a logical nav / activate / cancel code into whichever in-DLL overlay
// currently owns input. Priority mirrors input_poll_router's Win32 ordering
// exactly, so pad and keyboard resolve a contested key the same way. Returns
// true when an overlay took it.
bool RouteToOverlay(int logical) {
    // The F1 list is global (it opens over menus too) and owns its keys
    // outright while up, so it gets first refusal on every surface.
    if (acc::help::HandleNavCode(logical)) return true;

    // The KOTOR 1 quick menu is the mod's own overlay and equally global while
    // it is up — on KOTOR 2 the same surface is an engine panel and the
    // navigation chain walks it instead, so this declines there.
    if (acc::pad::quickmenu::HandleNavCode(logical)) return true;

    if (!InWorldSurface()) return false;

    if (acc::examine_view::IsActive()) {
        return acc::examine_view::HandleInputEvent(logical, 1);
    }
    if (acc::combat::queue::IsActive()) {
        return acc::combat::queue::HandleInputEvent(logical, 1);
    }
    if (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) {
        return acc::unified_menu::HandleInputEvent(logical, 1);
    }
    return false;
}

// LT — open the action menu, or close it if it is already up. The pad's whole
// action-menu vocabulary: one button, both directions, exactly what Shift+Enter
// and Esc are on the keyboard.
//
// Closing routes Esc into the menu rather than calling ForceDisarm, so the
// pause release, the queue commit and the close announcement all stay in
// CloseFromEsc — the same close B and the keyboard's Esc run. A menu the
// KEYBOARD opened closes here too: it is one menu, and a toggle that refused
// to close some of them would be a rule the user has to know.
void ToggleActionMenu() {
    if (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) {
        acclog::Write("Pad", "LT -> close action menu");
        acc::unified_menu::HandleInputEvent(kInputEsc1, 1);
        return;
    }
    acclog::Write("Pad", "LT -> open action menu");
    OpenActionMenu();
}

// The action queue's clear-all: a BARE Y while the queue is open.
//
// The keyboard clears the whole queue with Shift+Enter, and the queue's own
// handler tells that press from a plain Enter by reading the physical Shift. A
// pad holds no Shift, so the gesture has to be claimed here and call the queue
// directly.
//
// It used to be LT + A, which spent one of the eight trigger chords on a
// binding that could only ever fire on a single screen. The queue overlay is
// its own surface with its own vocabulary, and Y does nothing there — so Y is
// free, and taking it costs only the quick menu while the queue happens to be
// up. The result reads better too: inside the queue, A removes the entry you
// are on and Y removes them all.
//
// Returns true when it fired and the caller must consume; off the queue it
// declines instantly and Y keeps its ordinary job.
bool ClaimQueueClearAll() {
    if (!acc::combat::queue::IsActive()) return false;
    acclog::Write("Pad", "Y -> clear the action queue");
    acc::combat::queue::ClearAllAndClose();
    return true;
}

bool DispatchFaceChord(WORD button) {
    using A = acc::hotkeys::Action;
    const bool lt = g_ltHeld;
    const bool rt = g_rtHeld;
    if (!lt && !rt) return false;

    // One place decides which trigger is being held, so a chord can never mark
    // the wrong one and leave a stray solo job armed.
    bool& chorded = lt ? g_ltChorded : g_rtChorded;
    const char* side = lt ? "LT" : "RT";

    switch (button) {
        case kBtnA:
            chorded = true;
            if (lt) {
                acclog::Write("Pad", "LT + A -> announce the focused object");
                acc::cycle_input::DispatchPadAction(
                    acc::cycle_input::PadAction::AnnounceFocus);
            } else {
                acclog::Write("Pad", "RT + A -> look-around mode");
                acc::hotkeys::PadPress(A::ViewModeToggle);
            }
            return true;

        case kBtnB:
            chorded = true;
            if (lt) {
                acclog::Write("Pad", "LT + B -> the action queue");
                acc::hotkeys::PadPress(A::CombatQueueOpen);
            } else {
                acclog::Write("Pad", "RT + B -> copy the last spoken text");
                acc::hotkeys::PadPress(A::CopyLastSpoken);
            }
            return true;

        case kBtnX:
            // RT + X is deliberately empty — the grid's hardest reach, kept in
            // reserve rather than given a binding nobody would want there. It
            // is still claimed, so the engine cannot switch the party leader
            // under a held trigger.
            if (!lt) {
                chorded = true;
                acclog::Trace("Pad", "RT + X is unbound — claimed so bare X "
                                     "cannot fire under the trigger");
                return true;
            }
            chorded = true;
            acclog::Write("Pad", "LT + X -> own status");
            acc::hotkeys::PadPress(A::SelfStatusAnnounce);
            return true;

        case kBtnY:
            chorded = true;
            if (lt) {
                acclog::Write("Pad", "LT + Y -> cancel combat");
                FireGameAction(kActCancelCombat, kDikF, "LT + Y (cancel combat)");
            } else {
                acclog::Write("Pad", "RT + Y -> the level-up screen");
                acc::hotkeys::PadPress(A::LevelUpOpen);
            }
            return true;
    }
    return false;
}

// In-world D-Pad dispatch: the object cycle. The action menu never reaches
// here — while it is up it is an overlay and RouteToOverlay claims the code
// first. Returns true when the binding fired (the caller consumes).
bool DispatchWorldDpad(int code) {
    using PA = acc::cycle_input::PadAction;

    switch (code) {
        case kPadDpadLeft:  return acc::cycle_input::DispatchPadAction(PA::ItemPrev);
        case kPadDpadRight: return acc::cycle_input::DispatchPadAction(PA::ItemNext);
        case kPadDpadUp:    return acc::cycle_input::DispatchPadAction(PA::CategoryPrev);
        case kPadDpadDown:  return acc::cycle_input::DispatchPadAction(PA::CategoryNext);
        default:            return false;
    }
}

// The map screen's D-Pad.
//
// Every control on the map panel is filtered out of the chain (menus_chain
// drops the close button and both note-stepper buttons), so the D-Pad had
// nothing of ours to drive there and fell through to the engine — whose
// InGameMap steps prev / next map note on UP and DOWN, and does nothing at all
// with left and right. The hints were therefore on the vertical axis while the
// world's objects are on the horizontal one, and they are all one category:
// exactly what the horizontal axis is for everywhere else in the mod.
//
// So left and right step the hints through the mod's own Map cycle context —
// which is also what the keyboard's `,` / `.` already do here (spoken name,
// bearing and distance, with the virtual cursor panning to each).
//
// Up and down are NOT the category step they are in the world: the map has one
// category, so that step only ever repeated it. They decline instead and fall
// through to ordinary menu navigation, exactly the arrow keys' job on this
// screen — the chain walks the panel's real buttons (party selection, return
// to base, whatever the area offers).
//
// Gated on HasActiveMapPanel, the same predicate DispatchPadAction uses to
// pick Map over World context, so the two can never disagree about which
// cycle state the press lands in.
bool DispatchMapDpad(int code) {
    if (!acc::engine::HasActiveMapPanel()) return false;
    using PA = acc::cycle_input::PadAction;
    switch (code) {
        case kPadDpadLeft:  return acc::cycle_input::DispatchPadAction(PA::ItemPrev);
        case kPadDpadRight: return acc::cycle_input::DispatchPadAction(PA::ItemNext);
        default:            return false;
    }
}

// The shoulder chords: LT + LB speaks the route to the focused object, RT + RB walks
// to it. Returns true when the matching trigger is held and the chord fired —
// the caller's signal that the bare shoulder job must not also run. Marking the
// trigger chorded is what stops its own solo job firing on release.
//
// Shared by all three places a shoulder press arrives: KOTOR 1's poll, and on
// KOTOR 2 both the in-world client route and the manager route, which is the
// only one a shoulder reaches while a panel such as the map is up.
bool DispatchShoulderChord(bool right) {
    if (!right && g_ltHeld) {
        g_ltChorded = true;
        acclog::Write("Pad", "LT + LB -> route to focus");
        acc::cycle_input::DispatchPadAction(
            acc::cycle_input::PadAction::RouteFocus);
        return true;
    }
    if (right && g_rtHeld) {
        g_rtChorded = true;
        acclog::Write("Pad", "RT + RB -> walk to focus");
        acc::cycle_input::DispatchPadAction(
            acc::cycle_input::PadAction::WalkToFocus);
        return true;
    }
    return false;
}

}  // namespace acc::pad
