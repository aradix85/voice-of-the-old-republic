// Gamepad entry points of the input-poll slot: edge-detect the sampled
// button word (PollButtons), the buttons both games read from XInput
// (PollXInputButtons) and the trigger layer's solo jobs (PollTriggers).

#include "pad_input_internal.h"

#include "engine_panels.h"
#include "help.h"
#include "hotkeys.h"         // PadPress — reuse the keyboard actions' pollers
#include "log.h"
#include "minigame_pazaak.h" // TryHandlePadAction — the board owns Y and X
#include "minigame_swoop_race.h" // IsActive / TryShift / TryJump — the race owns A and B
#include "pad_quickmenu.h"   // the KOTOR 1 Y menu (K2 uses the engine's own)

namespace acc::pad {

namespace {

// Several frames at 30 fps, and still short enough that a press held for it
// reads as instant.
constexpr ULONGLONG kEnginePadProbeMs = 150;

// The XInput-sourced buttons' own edge baseline. Separate from g_btnLast
// because that one is PollButtons', which returns before touching it on
// KOTOR 2 — and this layer runs on both games.
WORD g_xiBtnLast = 0;

// D-Pad auto-repeat. KOTOR 2 gets this from the engine, which gates its own
// D-Pad codes behind a 150 ms repeat; without it here a KOTOR 1 player would
// have to tap once per entry down a long list.
constexpr ULONGLONG kDpadRepeatFirstMs = 400;
constexpr ULONGLONG kDpadRepeatNextMs  = 150;

struct DpadBit { WORD mask; int code; };
constexpr DpadBit kDpadBits[4] = {
    {kBtnDpadUp,    kPadDpadUp},
    {kBtnDpadDown,  kPadDpadDown},
    {kBtnDpadLeft,  kPadDpadLeft},
    {kBtnDpadRight, kPadDpadRight},
};
ULONGLONG g_dpadNextMs[4] = {0, 0, 0, 0};

// ---------------------------------------------------------------------------
// The D-Pad's second gesture: hold a direction to jump to the end
// ---------------------------------------------------------------------------
// The keyboard reaches the ends of a run in one press — Ctrl + comma / period
// for the nearest and farthest object, Home / End for the ends of a list. The
// pad had neither, and spending two more chords on them would have put the two
// commonest "get me out of this long list" gestures behind a modifier.
//
// So the D-Pad direction the player is ALREADY pressing carries it: keep
// holding past a second and the step becomes the jump. Nothing new to find and
// nothing to learn — a player who over-holds discovers it, and a player who
// taps never meets it.
//
// The jump fires once and then latches the direction until it is released, so
// the repeat that ran on the way there stops rather than walking back off the
// end. Both games consult that latch, at the two places their repeats come
// from: KOTOR 1's own repeat loop in PollButtons, and the engine's D-Pad events
// at TranslateManagerEvent on KOTOR 2.
constexpr ULONGLONG kDpadHoldJumpMs = 1000;

}  // namespace

void PollButtons() {
    const WORD now  = g_btnNow;
    const WORD last = g_btnLast;
    // Update the edge baseline BEFORE any gate below can return, so a button
    // held across an alt-tab, or across the open engine-pad question, does not
    // read as a fresh press.
    g_btnLast = now;

    // KOTOR 2's presses normally arrive as engine events at the two Translate*
    // seams, and dispatching them a second time from here would double every
    // binding. When its engine never saw the pad there are no such events, and
    // the mod is the driver on both games (EnginePad, pad_input_internal.h).
    if (g_enginePad != EnginePad::Dead) return;

    if (!Connected()) return;
    if (!acc::hotkeys::IsForegroundGame()) {
        for (ULONGLONG& t : g_dpadNextMs) t = 0;
        return;
    }

    const ULONGLONG nowMs = GetTickCount64();
    auto rising = [&](WORD m) { return (now & m) != 0 && (last & m) == 0; };

    // D-Pad first, and with repeat: it is the pad's primary surface in the
    // world and in every menu.
    for (int i = 0; i < 4; ++i) {
        const WORD m = kDpadBits[i].mask;
        if (!(now & m)) {
            g_dpadNextMs[i] = 0;
            continue;
        }
        // The hold already jumped to the end on this direction. Stop stepping
        // until it is released, or the repeat would walk straight back off it.
        if (g_dpadJumped[i]) continue;
        if (!(last & m)) {
            g_dpadNextMs[i] = nowMs + kDpadRepeatFirstMs;
            DispatchDpad(kDpadBits[i].code);
            continue;
        }
        if (nowMs >= g_dpadNextMs[i]) {
            g_dpadNextMs[i] = nowMs + kDpadRepeatNextMs;
            DispatchDpad(kDpadBits[i].code);
        }
    }

    // A face button under a trigger is the chord grid's, dispatched from
    // PollXInputButtons. Its bare job must not also run. LT + A is the
    // exception DispatchA already handles: it claims the queue's clear-all
    // first and falls back to the ordinary interact when the queue is closed.
    const bool triggerHeld = g_ltHeld || g_rtHeld;

    // A swoop race owns the face buttons, with the bindings KOTOR 2's own
    // engine gives its pad: B shifts up, A jumps (KOTOR 1 has no jump, so A is
    // idle there). Every other button's world job is suppressed for the race —
    // Start would press the pause key, which is Space, KOTOR 1's shift; the
    // right stick press would hold a turn key; X, the left stick press and the
    // shoulders act on a party and targets the race does not have.
    if (acc::swoop_race::IsActive()) {
        if (rising(kBtnB) && !triggerHeld) {
            acclog::Write("Pad", "B -> swoop race shift");
            acc::swoop_race::TryShift();
        }
        if (rising(kBtnA) && !triggerHeld) {
            if (acc::swoop_race::TryJump()) {
                acclog::Write("Pad", "A -> swoop race jump");
            } else {
                acclog::Write("Pad", "A in the swoop race — no jump on this "
                                     "game, ignored");
            }
        }
        return;
    }

    if (rising(kBtnA) && !g_rtHeld) DispatchA();
    if (rising(kBtnB) && !triggerHeld) DispatchB();
    if (rising(kBtnShoulderL)) DispatchShoulder(/*right=*/false);
    if (rising(kBtnShoulderR)) DispatchShoulder(/*right=*/true);

    // The Pazaak board claims Y and X ahead of both of their ordinary jobs. It
    // is a foreground panel, so the world-only gate further down would drop
    // both presses on the floor there, and Y's quick menu acts on a world the
    // player has stepped away from. Declines instantly off the board.
    if (rising(kBtnY) && !triggerHeld) {
        // Y is claimed in order by the two surfaces that own it while they are
        // up — the action queue clears itself, the Pazaak board stands — and is
        // the quick menu everywhere else. That menu opens only from the world
        // (it acts on the world) but closes from wherever it happens to be up.
        if (!ClaimQueueClearAll() &&
            !acc::pazaak::TryHandlePadAction(acc::pazaak::PadAction::Stand) &&
            (acc::pad::quickmenu::IsOpen() || InWorldSurface())) {
            acc::pad::quickmenu::Toggle();
        }
    }
    if (rising(kBtnX) &&
        acc::pazaak::TryHandlePadAction(acc::pazaak::PadAction::EndTurn)) {
        return;   // the board took it; nothing below applies on a panel
    }

    // The remaining buttons are engine actions with no mod surface of their
    // own, so they are world-only: in a menu the D-Pad and A/B are the whole
    // vocabulary, and a stray party-leader switch there would be a surprise.
    if (!InWorldSurface()) return;

    // X only switches the leader BARE — with a trigger held it is the mod's H
    // key, dispatched by PollXInputButtons.
    if (rising(kBtnX) && !triggerHeld) {
        FireGameAction(kActChangeChar, kDikTab, "X (switch party leader)");
    }
    if (rising(kBtnBack))    FireGameAction(kActOptions, kDikO,
                                            "Back (options)");
    if (rising(kBtnStart))   FireGameAction(kActPauseSpace, kDikSpace,
                                            "Start (pause)");
    if (rising(kBtnThumbL))  FireGameAction(kActFlourish, kDikX,
                                            "left stick press (flourish)");
    if (rising(kBtnThumbR)) {
        // No MOUSE_BUTTON1 hazard here — that trap is K2's, where the engine
        // hands R3 to us as the same InputIndex the right mouse button uses.
        // We read the physical button, so there is nothing to disambiguate.
        acclog::Write("Pad", "right stick press -> camera orient");
        acc::hotkeys::PadPress(acc::hotkeys::Action::CameraOrient);
    }
}

bool PeekModifierHeld() { return Connected() && g_yHeld; }

void PollXInputButtons() {
    // Both games. Own edge baseline, updated BEFORE any gate below can return:
    // a button held across an alt-tab must not read as a fresh press on the
    // way back.
    const WORD now  = g_btnNow;
    const WORD last = g_xiBtnLast;
    g_xiBtnLast = now;

    if (!Connected()) return;
    if (!acc::hotkeys::IsForegroundGame()) {
        // A direction held when focus was lost must not read as a continuing
        // hold on the way back, or alt-tabbing mid-press would jump. The
        // engine-pad probe goes with it: a deadline that expires while the game
        // is not foreground proves nothing about the engine, which is not being
        // sent events either.
        for (int i = 0; i < 4; ++i) { g_dpadHoldSince[i] = 0; g_dpadJumped[i] = false; }
        g_probeCode     = 0;
        g_probeSlot     = -1;
        g_probeDeadline = 0;
        return;
    }

    const ULONGLONG nowMs = GetTickCount64();

    // The engine-pad probe. Runs only while the question is open, which on
    // KOTOR 1 is never and on a healthy KOTOR 2 is until the first D-Pad press.
    if (g_enginePad == EnginePad::Unknown) {
        if (g_probeCode == 0) {
            for (int i = 0; i < 4; ++i) {
                if ((now & kDpadBits[i].mask) == 0 ||
                    (last & kDpadBits[i].mask) != 0) {
                    continue;
                }
                g_probeCode     = kDpadBits[i].code;
                g_probeSlot     = i;
                g_probeDeadline = nowMs + kEnginePadProbeMs;
                acclog::Write("Pad", "probing the engine's gamepad path with "
                                     "D-Pad(%d) — holding %llu ms for its event",
                              g_probeCode,
                              static_cast<unsigned long long>(kEnginePadProbeMs));
                break;
            }
        } else if (nowMs >= g_probeDeadline) {
            // Silence. The engine never saw this pad — almost always because it
            // was switched on after the game's one enumeration at input init.
            g_enginePad = EnginePad::Dead;
            acclog::Write("Pad", "engine gamepad path is DEAD (no event for "
                                 "D-Pad(%d)) — the mod drives the pad from here, "
                                 "as it does on KOTOR 1", g_probeCode);
            // Pay the probed press, and stamp the repeat clock so PollButtons
            // does not immediately fire it a second time on its own baseline.
            if (g_probeSlot >= 0) g_dpadNextMs[g_probeSlot] = nowMs + kDpadRepeatFirstMs;
            DispatchDpad(g_probeCode);
            g_probeCode = 0;
            g_probeSlot = -1;
        }
    }

    // The D-Pad hold. Sampled here rather than in Tick() because what it fires
    // is a DISPATCH, and every dispatch the pad makes belongs in the input-poll
    // slot — the same rule the trigger layer follows.
    for (int i = 0; i < 4; ++i) {
        if (!(now & kDpadBits[i].mask)) {
            g_dpadHoldSince[i] = 0;
            g_dpadJumped[i]    = false;
            continue;
        }
        if (g_dpadHoldSince[i] == 0) { g_dpadHoldSince[i] = nowMs; continue; }
        if (g_dpadJumped[i]) continue;
        if (nowMs - g_dpadHoldSince[i] < kDpadHoldJumpMs) continue;
        g_dpadJumped[i] = true;
        DispatchDpadHold(kDpadBits[i].code);
    }

    // Press the right stick on the map screen: drop a marker at the cursor,
    // the keyboard's Shift + N (in the world the same press turns the camera
    // to the next exit). Claimed from XInput because neither game delivers a
    // stick press on a menu surface: KOTOR 1's own R3 handling sits behind a
    // world-only gate and KOTOR 2's arrives on the in-world client route, so
    // there is nothing to collide with here.
    if ((now & kBtnThumbR) != 0 && (last & kBtnThumbR) == 0 &&
        acc::engine::HasActiveMapPanel()) {
        acclog::Write("Pad", "press right stick on the map -> drop marker");
        acc::hotkeys::PadPress(acc::hotkeys::Action::SaveMarkerAtCursor);
    }

    // The left stick press is the engine's Flourish Weapon in the world, and
    // idle on the Pazaak board — where it reads how many cards the opponent
    // still holds, the keyboard's Shift+C. Claimed from XInput because neither
    // game delivers that press on a menu surface: KOTOR 1 gates its own handling
    // on the world and KOTOR 2 receives it on the in-world client route.
    bool stickLClaimed = false;
    if ((now & kBtnThumbL) != 0 && (last & kBtnThumbL) == 0) {
        stickLClaimed =
            acc::pazaak::TryHandlePadAction(acc::pazaak::PadAction::OppHand);
    }

    // The face-button chord grid. Claimed here rather than from the engine's own
    // button events because only XInput knows whether a trigger is down, and
    // because these run on every surface — the status readout is as useful in
    // the inventory as in the world. A bare face button falls through untouched.
    static const WORD kFaceButtons[] = {kBtnA, kBtnB, kBtnX, kBtnY};
    WORD chordClaimed = 0;
    for (const WORD b : kFaceButtons) {
        if ((now & b) == 0 || (last & b) != 0) continue;
        if (DispatchFaceChord(b)) chordClaimed |= b;
    }

    // Name the presses this layer does NOT claim, one line per press.
    //
    // Start and L3 have no GUI-class mapping on KOTOR 2 at all: press either in
    // a menu and the engine delivers nothing, so the log stays silent and
    // "which button did I just press" is unanswerable from it — which is
    // precisely the question that has to be settled when a binding moves. XInput
    // sees every button on every screen, so one line here closes that gap.
    // Bounded by construction: rising edges only.
    //
    // What it CANNOT see is the Series pad's Share button: Windows keeps that
    // one for the Game Bar capture and reports it through no input API we have.
    // Its absence from this log is therefore expected, not a missed press.
    struct Named { WORD mask; const char* name; };
    static const Named kUnclaimed[] = {
        {kBtnStart, "Start"}, {kBtnBack, "Back"}, {kBtnX, "X"},
        {kBtnThumbL, "left stick press"},
    };
    for (const Named& b : kUnclaimed) {
        if ((b.mask & chordClaimed) != 0) continue;   // the chord above took it
        if (b.mask == kBtnThumbL && stickLClaimed) continue;   // the board's
        if ((now & b.mask) != 0 && (last & b.mask) == 0) {
            acclog::Write("Pad", "%s pressed (XInput bit 0x%04x) — this button "
                                 "is the game's, not the mod's", b.name,
                          static_cast<unsigned>(b.mask));
        }
    }
}

void PollTriggers() {
    const bool lt   = g_pendingLtSolo;
    const bool rt   = g_pendingRtSolo;
    const bool help = g_pendingHelp;
    // Clear before acting, and before any gate below can return: a job the
    // game was not foreground for by the time we got here is dropped, not
    // carried into the next tick.
    g_pendingLtSolo = g_pendingRtSolo = g_pendingHelp = false;
    if (!lt && !rt && !help) return;
    if (!acc::hotkeys::IsForegroundGame()) return;

    if (help) {
        // In a dialogue the screen's keys are not the question — the line that
        // just went past is, and a pad has no R to ask with. Same chord, the
        // answer the surface actually owes. Routed as a synthetic press so the
        // repeat keeps its one implementation in dialog_speech, which polls
        // ahead of the other two readers of this action and, with a dialog
        // panel foreground, is the only one whose gate passes.
        if (acc::engine::HasActiveDialogPanel()) {
            acclog::Write("Pad", "LT + RT in dialogue -> repeat the line");
            acc::hotkeys::PadPress(acc::hotkeys::Action::DialogRepeatLine);
        } else {
            acclog::Write("Pad", "LT + RT -> context help");
            acc::help::SpeakContextHelp();
        }
    }
    // LT is the action menu's open AND close, so it is world-only in both
    // directions: in a menu the pad is simply the keyboard, and there is no
    // action menu to toggle over an engine panel. A suspended menu is exactly
    // that case — a blocking panel is up, so InWorldSurface() is already false.
    // Not in a swoop race either: there is nothing to act on, and the race
    // runs on regardless.
    if (lt && InWorldSurface() && !acc::swoop_race::IsActive()) {
        NoteNavPress();
        ToggleActionMenu();
    }
    // RT — routed as a synthetic press so announce_facing keeps its own
    // world-vs-map branch rather than the pad growing a second copy.
    if (rt) {
        acclog::Write("Pad", "RT -> announce facing");
        acc::hotkeys::PadPress(acc::hotkeys::Action::AnnounceDegrees);
    }
}

}  // namespace acc::pad
