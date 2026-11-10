// Gamepad presses the mod dispatches itself (KOTOR 1, and KOTOR 2 when its
// engine never saw the pad): engine actions and menu keys sent as scancodes,
// the D-Pad, A, B and the shoulders, and the D-Pad hold that jumps to the
// end of a run.

#include "pad_input_internal.h"

#include "cycle_input.h"
#include "engine_input.h"
#include "engine_keymap.h"   // GameActionScancode — KOTOR 1 fires engine
                             // actions by synthesising the player's own bind
#include "engine_panels.h"
#include "key_inject.h"      // Send / Tap — the scancode route into the engine
#include "log.h"
#include "minigame_pazaak.h" // TryHandlePadAction — the board owns Y and X

namespace acc::pad {

namespace {

// Navigate an ENGINE panel: send the arrow / Enter / Esc the keyboard would
// have sent. Nothing else in the mod needs to know a pad exists — the press
// arrives at the GUI-manager hook as an ordinary keyboard event and the
// navigation chain handles it exactly as it handles the keyboard, including
// the press/release pairing and the engine's own control focus.
//
// If the arrow cluster ever turns out not to reach the engine's DirectInput
// read (the extended-scancode question), the fallback is a direct call into
// acc::menus::chain::HandleNavStep / HandleLeftRight / HandleEnterActivation /
// HandleEsc, which are exported for exactly this shape of caller.
void InjectMenuKey(int logical) {
    switch (logical) {
        case kInputNavUp:    acc::key_inject::Tap(kDikUp,    true); break;
        case kInputNavDown:  acc::key_inject::Tap(kDikDown,  true); break;
        case kInputNavLeft:  acc::key_inject::Tap(kDikLeft,  true); break;
        case kInputNavRight: acc::key_inject::Tap(kDikRight, true); break;
        case kInputHome:     acc::key_inject::Tap(kDikHome,  true); break;
        case kInputEnd:      acc::key_inject::Tap(kDikEnd,   true); break;
        case kInputEnter1:   acc::key_inject::Tap(kDikReturn);      break;
        case kInputEsc1:     acc::key_inject::Tap(kDikEscape);      break;
        default: return;
    }
    acclog::Trace("Pad", "menu key injected for logical %d", logical);
}

int DpadSlotForCode(int code) {
    switch (code) {
        case kPadDpadUp:    return 0;
        case kPadDpadDown:  return 1;
        case kPadDpadLeft:  return 2;
        case kPadDpadRight: return 3;
        default:            return -1;
    }
}

}  // namespace

// Fire a [Keymapping] game action by synthesising the key it is bound to.
void FireGameAction(int actionId, int defaultDik, const char* what) {
    int scan = acc::engine_keymap::GameActionScancode(actionId);
    if (scan == 0) scan = defaultDik;
    acclog::Write("Pad", "%s -> game action %d (scan 0x%02x)", what, actionId,
                  scan);
    acc::key_inject::Tap(scan);
}

// The logical code a pad D-Pad / stick direction stands in for.
int LogicalForDirection(int code) {
    switch (code) {
        case kPadDpadUp:    return kInputNavUp;
        case kPadDpadDown:  return kInputNavDown;
        case kPadDpadLeft:  return kInputNavLeft;
        case kPadDpadRight: return kInputNavRight;
        default:            return 0;
    }
}

// ---------------------------------------------------------------------------
// One press, dispatched by the mod
// ---------------------------------------------------------------------------
// Each of these is the twin of a branch in TranslateManagerEvent, in the same
// order of refusal, calling the same helpers. What differs is only the tail:
// where a live KOTOR 2 engine pad rewrites the code and lets the engine's own
// dispatch carry it on, these synthesise the key the keyboard would have sent.

void DispatchDpad(int code) {
    const int logical = LogicalForDirection(code);
    NoteNavPress();
    if (RouteToOverlay(logical)) {
        acclog::Write("Pad", "D-Pad -> overlay logical %d", logical);
        return;
    }
    if (InWorldSurface()) {
        // In the world the D-Pad is ours outright, and the fall-through must
        // NOT reach the injection below: KOTOR 1 binds the arrow keys as the
        // movement alternates (Action285/286), so an injected Up there would
        // walk the character. A cycle that declines (empty category, no
        // context) is silence, not a step forward.
        DispatchWorldDpad(code);
        return;
    }
    if (DispatchMapDpad(code)) {
        acclog::Write("Pad", "D-Pad -> map cycle");
        return;
    }
    InjectMenuKey(logical);
}

void DispatchA() {
    if (!ClaimA()) InjectMenuKey(kInputEnter1);
}

void DispatchB() {
    NoteNavPress();
    if (RouteToOverlay(kInputEsc1)) {
        acclog::Write("Pad", "B -> overlay cancel");
        return;
    }
    // With no overlay up there is nothing to cancel in the world: KOTOR 1's
    // Escape opens the in-game menu, and that is the quick menu's first entry,
    // not a button the player should hit by reflex while walking. (The action
    // menu is an overlay, so B closes it at RouteToOverlay above — the second
    // way out of it, beside LT.)
    if (InWorldSurface()) return;
    InjectMenuKey(kInputEsc1);
}

void DispatchShoulder(bool right) {
    // The Pazaak board first. The shoulders are idle there — the engine's
    // sub-screen step has no sub-screens to reach — so they carry the board's
    // two readers, the keyboard's C and T.
    if (acc::pazaak::TryHandlePadAction(right ? acc::pazaak::PadAction::ReviewTable
                                              : acc::pazaak::PadAction::ReviewHand)) {
        return;
    }

    // The chords come first — a shoulder taken during a trigger hold is the
    // chord, never the sub-screen step or the target cycle.
    if (DispatchShoulderChord(right)) return;

    if (DispatchShoulderModeToggle(right)) return;

    // Otherwise the shoulders do what they do on KOTOR 2: cycle the target in
    // the world, and step the in-game menu's sub-screens everywhere else —
    // which is how a pad reaches the map, the journal and the equipment screen
    // at all. KOTOR 1 has both as ordinary [Keymapping] actions.
    if (InWorldSurface()) {
        FireGameAction(right ? kActSelectNext : kActSelectPrev,
                       right ? kDikE : kDikQ,
                       right ? "RB (cycle target right)"
                             : "LB (cycle target left)");
        return;
    }
    FireGameAction(right ? kActNextMenu : kActPrevMenu,
                   right ? kDikE : kDikQ,
                   right ? "RB (next sub-screen)" : "LB (previous sub-screen)");
}

// True while this direction's hold has already spent its jump. The step paths
// skip the direction entirely until the player lets go.
bool DpadHoldSpent(int code) {
    const int slot = DpadSlotForCode(code);
    return slot >= 0 && g_dpadJumped[slot];
}

// Pay the hold. Which jump it is depends on what the D-Pad is driving, and
// that is the same two-way split every other D-Pad path here already makes.
void DispatchDpadHold(int code) {
    using PA = acc::cycle_input::PadAction;

    // The object cycle — in the world, and on the map screen, which runs the
    // same cycle over its hints. Left is the nearest object and right the
    // farthest: the two ends of the very run the taps were stepping through,
    // and exactly what Ctrl + comma / period reach on the keyboard. In the
    // world up and down step the CATEGORY, a short ring with no end worth
    // jumping to, so they keep their repeat and nothing more. On the map up
    // and down walk the panel's buttons (DispatchMapDpad), so their hold is the
    // list's Home / End below.
    const bool mapHorizontal = acc::engine::HasActiveMapPanel() &&
                               (code == kPadDpadLeft || code == kPadDpadRight);
    if (InWorldSurface() || mapHorizontal) {
        if (code == kPadDpadLeft) {
            acclog::Write("Pad", "D-Pad left held -> nearest object");
            acc::cycle_input::DispatchPadAction(PA::ItemFirst);
        } else if (code == kPadDpadRight) {
            acclog::Write("Pad", "D-Pad right held -> farthest object");
            acc::cycle_input::DispatchPadAction(PA::ItemLast);
        }
        return;
    }

    // A list: up is its first row, down its last — the keyboard's Home and End.
    //
    // Injected as the real key rather than routed as a synthetic action press,
    // because a list can have more than one reader in a single tick (the F1
    // overlay standing over an engine panel is the ordinary case) and a
    // synthetic press is taken by whichever of them polls first. A real Home /
    // End reaches every one of them, exactly as the player's own key would.
    if (code == kPadDpadUp) {
        acclog::Write("Pad", "D-Pad up held -> first entry");
        NoteNavPress();
        InjectMenuKey(kInputHome);
    } else if (code == kPadDpadDown) {
        acclog::Write("Pad", "D-Pad down held -> last entry");
        NoteNavPress();
        InjectMenuKey(kInputEnd);
    }
}

}  // namespace acc::pad
