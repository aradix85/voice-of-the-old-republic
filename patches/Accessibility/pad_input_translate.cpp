// KOTOR 2's two gamepad seams: normalise pad events at the GUI manager
// (TranslateManagerEvent) and claim chords on the in-world client route
// (TranslateClientEvent), plus telling a pad event from its keyboard twin.

#include "pad_input_internal.h"

#include "engine_game.h"
#include "engine_input.h"
#include "hotkeys.h"         // PadPress — reuse the keyboard actions' pollers
#include "log.h"
#include "map_ui_cursor.h"   // IsActive — the map screen owns the left stick
#include "minigame_pazaak.h" // TryHandlePadAction — the board owns Y and X
#include "minigame_swoop_race.h" // IsActive / TryShift / TryJump — the race owns A and B

namespace acc::pad {

namespace {

// ---------------------------------------------------------------------------
// Axis release pairing
// ---------------------------------------------------------------------------
// Codes 51 / 52 send val = 0 on centring and that release carries no
// direction. Keep the last direction per axis so the release can be rewritten
// to the SAME logical code the press became — otherwise the press-release
// pairing in OnHandleInputEvent mismatches and a later real press gets
// swallowed. The D-Pad needs none of this: it sends val = 1 only, never a
// release.
int g_axisLatch[2] = {0, 0};   // [0] = vertical (51), [1] = horizontal (52)

// ---------------------------------------------------------------------------
// Pad-vs-keyboard discrimination
// ---------------------------------------------------------------------------
bool PhysDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// The keyboard row each shared code belongs to. If that physical key is down
// right now, the event is the keyboard's, not the pad's.
//
// The engine drops unbound scancodes before our hook, and stock [Keymapping]
// binds none of F9..F12 — so in practice these codes can only be the pad. The
// physical-key test closes the remaining case (a player who bound F9..F12 in
// the game's own key-mapping screen) at the cost of one OS call per event.
int KeyboardTwinVk(int code) {
    switch (code) {
        case kPadDpadLeft:   return VK_F9;
        case kPadDpadRight:  return VK_F10;
        case kPadDpadUp:     return VK_F11;
        case kPadDpadDown:   return VK_F12;
        case kPadStickVert:  return 'A';
        case kPadStickHoriz: return 'B';
        case kPadButtonA:    return VK_F1;
        case kPadButtonB:    return VK_F2;
        case kPadButtonX:    return VK_F3;
        case kPadButtonY:    return VK_F4;
        case kPadShoulderL:  return 'C';
        case kPadShoulderR:  return 'D';
        case kPadBack:       return VK_F8;
        default:             return 0;
    }
}

}  // namespace

bool IsPadCode(int code) {
    if (!acc::game::IsKotor2()) return false;
    switch (code) {
        case kPadDpadLeft: case kPadDpadRight:
        case kPadDpadUp:   case kPadDpadDown:
        case kPadStickVert: case kPadStickHoriz:
        case kPadButtonA:  case kPadButtonB:
        case kPadButtonX:  case kPadButtonY:
        case kPadShoulderL: case kPadShoulderR:
        case kPadBack:
            return true;
        default:
            return false;
    }
}

const char* CodeHint(int code) {
    if (!acc::game::IsKotor2()) return nullptr;
    switch (code) {
        case kPadDpadLeft:   return "D-Pad left";
        case kPadDpadRight:  return "D-Pad right";
        case kPadDpadUp:     return "D-Pad up";
        case kPadDpadDown:   return "D-Pad down";
        case kPadStickVert:  return "left stick vertical";
        case kPadStickHoriz: return "left stick horizontal";
        case kPadButtonA:    return "A";
        case kPadButtonB:    return "B";
        case kPadButtonX:    return "X";
        case kPadButtonY:    return "Y";
        case kPadShoulderL:  return "LB";
        case kPadShoulderR:  return "RB";
        case kPadBack:       return "Back";
        default:             return nullptr;
    }
}

bool IsPhysicalF1() {
    if (!acc::game::IsKotor2()) return true;  // no pad path — 0x27 is F1
    return PhysDown(VK_F1);
}

bool TranslateClientEvent(int code, int value) {
    if (!acc::game::IsKotor2()) return false;
    // These three codes are ordinary engine InputIndices that a keyboard and a
    // mouse also produce. Nothing here may fire in a session that has never
    // seen a controller.
    if (!Connected()) return false;
    if (value == 0) return false;

    switch (code) {
        case kPadClientShoulderL:
        case kPadClientShoulderR:
            // Bare LB / RB stay the engine's Cycle Target; only the chord is
            // ours, and consuming it stops the engine ALSO cycling.
            return DispatchShoulderChord(code == kPadClientShoulderR);

        case kPadClientStickR:
            // R3 shares InputIndex 1 with MOUSE_BUTTON1 — the RIGHT MOUSE
            // BUTTON, i.e. mouse-look. Consuming it blindly would take
            // right-click away from every KOTOR 2 player with a pad plugged
            // in. The physical-button test is the same twin-key discriminator
            // the manager codes use: button actually down means the mouse
            // sent it, so stand down.
            if (PhysDown(VK_RBUTTON)) return false;
            // The orient holds a turn key, and in a swoop race that steers.
            if (acc::swoop_race::IsActive()) {
                acclog::Write("Pad", "right stick press in the swoop race — ignored");
                return true;
            }
            acclog::Write("Pad", "right stick press -> camera orient");
            acc::hotkeys::PadPress(acc::hotkeys::Action::CameraOrient);
            return true;

        default:
            return false;
    }
}

Verdict TranslateManagerEvent(int& code, int& value) {
    if (!acc::game::IsKotor2()) return Verdict::NotPad;
    if (!IsPadCode(code)) return Verdict::NotPad;

    // A physically-held twin key means this is the keyboard's event, not the
    // pad's. Runs before anything else so a bound F9..F12 / A / B / F1 / F2
    // keeps its vanilla meaning.
    const int twinVk = KeyboardTwinVk(code);
    if (twinVk != 0 && PhysDown(twinVk)) {
        // Traced, not silent. This is the correct answer whenever the keyboard
        // really did send the event — but it is ALSO what a handheld in desktop
        // / keyboard-emulation mode produces: its buttons arrive as genuine
        // keystrokes, the twin is therefore genuinely down, and every pad event
        // gets rejected here. Without a line, that failure reads as "the
        // controller does nothing" with nothing in the log to say why. Trace
        // rather than Write: on a keyboard-only session this fires on ordinary
        // typing, and the dedup collapses it.
        acclog::Trace("Pad",
                      "engine code %d ignored — its keyboard twin (VK 0x%02X) "
                      "is physically down, so this is the keyboard's event",
                      code, twinVk);
        return Verdict::NotPad;
    }

    const int raw = code;

    // ---- Swoop race: every pad input is the engine's ------------------------
    // KOTOR 2's race is fully pad-driven by the engine itself — the left stick
    // steers, and A / B reach the client input hook as 0xd9 / 0x16, where
    // input_pipeline runs them through TryJump / TryShift. Nothing here may
    // turn A into an interact or B into Esc while a race is up.
    if (acc::swoop_race::IsActive()) {
        acclog::Trace("Pad", "engine code %d left to the engine — swoop race", raw);
        return Verdict::NotPad;
    }

    // ---- Left stick -> nav ------------------------------------------------
    // The engine's own preamble keys on val == +/-1 for these two codes, so
    // that test is engine-native and cannot drift. val == 0 is the centring
    // release; it carries no direction, so it takes the latched one.
    if (code == kPadStickVert || code == kPadStickHoriz) {
        const int axis = (code == kPadStickVert) ? 0 : 1;
        int logical = 0;
        if (value == -1) {
            logical = (axis == 0) ? kInputNavUp : kInputNavLeft;
        } else if (value == 1) {
            logical = (axis == 0) ? kInputNavDown : kInputNavRight;
        } else if (value == 0 && g_axisLatch[axis] != 0) {
            // Release of a press we rewrote. Hand back the SAME logical code
            // so the press-release pairing matches, then clear the latch.
            code = g_axisLatch[axis];
            g_axisLatch[axis] = 0;
            acclog::Write("Pad", "stick axis %d release -> logical %d",
                          axis, code);
            return Verdict::Rewritten;
        } else {
            return Verdict::NotPad;   // stray centring with nothing latched
        }
        NoteSeen();
        g_axisLatch[axis] = logical;

        // In the world the left stick MOVES the character; it must never
        // double as menu navigation there. (Movement itself never reaches this
        // hook — it is the engine's own walk path.)
        if (InWorldSurface()) {
            g_axisLatch[axis] = 0;
            return Verdict::NotPad;
        }

        // On the map screen the left stick pans the virtual cursor, exactly as
        // the player's walk keys do there — that is the analog surface the map
        // most wants, and the D-Pad still covers menu navigation. Swallow the
        // axis event so the engine cannot ALSO step the panel's controls with
        // it; the pan itself comes from the XInput sample in Tick(), read by
        // map_ui_cursor.
        if (acc::map_ui_cursor::IsActive()) {
            g_axisLatch[axis] = 0;
            acclog::Trace("Pad", "stick(%d) consumed — map cursor owns it", raw);
            return Verdict::Consumed;
        }

        NoteNavPress();
        if (RouteToOverlay(logical)) return Verdict::Consumed;
        code = logical;
        acclog::Write("Pad", "stick(%d) val=%d -> logical %d", raw, value, code);
        return Verdict::Rewritten;
    }

    // ---- D-Pad ------------------------------------------------------------
    const int dpadLogical = LogicalForDirection(code);
    if (dpadLogical != 0) {
        NoteSeen();
        if (value == 0) return Verdict::NotPad;   // no release is ever sent
        // The hold gesture already jumped to the end on this direction, and
        // the engine keeps sending its repeat. Swallow it until release —
        // KOTOR 1 skips the same direction in its own repeat loop.
        if (DpadHoldSpent(code)) return Verdict::Consumed;
        NoteNavPress();
        if (RouteToOverlay(dpadLogical)) {
            acclog::Write("Pad", "D-Pad(%d) -> overlay logical %d",
                          raw, dpadLogical);
            return Verdict::Consumed;
        }
        if (InWorldSurface() && DispatchWorldDpad(code)) {
            return Verdict::Consumed;
        }
        if (DispatchMapDpad(code)) {
            acclog::Write("Pad", "D-Pad(%d) -> map cycle", raw);
            return Verdict::Consumed;
        }
        code = dpadLogical;
        acclog::Write("Pad", "D-Pad(%d) -> logical %d", raw, code);
        return Verdict::Rewritten;
    }

    // ---- The chord grid gets first refusal on every face button ------------
    // A face button pressed while a trigger is held belongs to the grid, which
    // is dispatched from the XInput sample — the only reader that can see a
    // trigger at all. All this route has to do is stop the engine, and the bare
    // handlers below, acting on the same press.
    if (value != 0 && (g_ltHeld || g_rtHeld) &&
        (code == kPadButtonA || code == kPadButtonB ||
         code == kPadButtonX || code == kPadButtonY)) {
        acclog::Trace("Pad", "face button %d consumed — trigger held, the "
                             "chord grid owns it", code);
        return Verdict::Consumed;
    }

    // ---- A ----------------------------------------------------------------
    if (code == kPadButtonA) {
        if (value == 0) return Verdict::NotPad;
        // Consumed when claimed, so the engine's own default action cannot
        // also fire on a stale target.
        if (ClaimA()) return Verdict::Consumed;
        code = kInputEnter1;
        acclog::Write("Pad", "A -> logical Enter");
        return Verdict::Rewritten;
    }

    // ---- B ----------------------------------------------------------------
    if (code == kPadButtonB) {
        if (value == 0) return Verdict::NotPad;
        NoteNavPress();
        if (RouteToOverlay(kInputEsc1)) {
            acclog::Write("Pad", "B -> overlay cancel");
            return Verdict::Consumed;
        }
        // With no overlay up (the action menu is one, and RouteToOverlay closed
        // it above if it was), the world's B stays the engine's own cancel.
        if (InWorldSurface()) return Verdict::NotPad;
        // In a menu, B is Esc. Our close-and-announce path gets it; if it
        // declines, the engine still closes the panel off the raw 0x28 it
        // reads from its own frame.
        code = kInputEsc1;
        acclog::Write("Pad", "B -> logical Esc");
        return Verdict::Rewritten;
    }

    // ---- LB / RB ----------------------------------------------------------
    // The bare shoulders are the engine's nearly everywhere: in the world they
    // cycle targets, and on the in-game menu strip they step between the
    // sub-screens, which is how the pad reaches the map at all. The container
    // and the store are the exception (DispatchShoulderModeToggle).
    if (code == kPadShoulderL || code == kPadShoulderR) {
        if (value == 0) return Verdict::NotPad;
        // The Pazaak board's two readers, same as KOTOR 1's.
        if (acc::pazaak::TryHandlePadAction(
                code == kPadShoulderR ? acc::pazaak::PadAction::ReviewTable
                                      : acc::pazaak::PadAction::ReviewHand)) {
            return Verdict::Consumed;
        }
        // The chords, off the world. In the world the shoulder arrives on the
        // client route too and TranslateClientEvent takes the chord there, so
        // claiming it here as well would fire it twice. On a panel this is the
        // only route; without it LT + LB on the map would reach the engine as
        // a bare LB and step to the next sub-screen.
        if (!InWorldSurface() && DispatchShoulderChord(code == kPadShoulderR)) {
            return Verdict::Consumed;
        }
        return DispatchShoulderModeToggle(code == kPadShoulderR)
                   ? Verdict::Consumed
                   : Verdict::NotPad;
    }

    // ---- Y ----------------------------------------------------------------
    // In the world Y stays the engine's Quick Menu. In a menu it is the mod's
    // peek modifier — hold it and the D-Pad's up / down read the focused entry's
    // description, the pad's Shift+arrow. The modifier itself is read from
    // XInput (PeekModifierHeld), so this event has no job left; consuming it
    // keeps the engine from finding one of its own on a screen where the pad is
    // meant to be the keyboard.
    if (code == kPadButtonY) {
        if (value == 0) return Verdict::NotPad;
        // The two surfaces that own Y while they are up, in the same order the
        // KOTOR 1 poll takes them: the action queue clears itself, the Pazaak
        // board stands. On the board the peek modifier has nothing to peek at
        // either — that navigator owns the D-Pad.
        if (ClaimQueueClearAll()) return Verdict::Consumed;
        if (acc::pazaak::TryHandlePadAction(acc::pazaak::PadAction::Stand)) {
            return Verdict::Consumed;
        }
        if (InWorldSurface()) return Verdict::NotPad;
        acclog::Trace("Pad", "Y consumed in a menu — it is the peek modifier there");
        return Verdict::Consumed;
    }

    // ---- X -----------------------------------------------------------------
    // On the Pazaak board bare X is End turn; switching the party leader there
    // is meaningless, the board is the whole game while it is up. Everywhere
    // else bare X stays the engine's Switch Party Leader.
    if (code == kPadButtonX && value != 0 &&
        acc::pazaak::TryHandlePadAction(acc::pazaak::PadAction::EndTurn)) {
        return Verdict::Consumed;
    }

    // Everything else (bare X, Back) keeps its engine meaning. IsPadCode still
    // flags them, so the F1 suppression in OnHandleInputEvent and the log
    // annotation know what they are looking at.
    return Verdict::NotPad;
}

}  // namespace acc::pad
