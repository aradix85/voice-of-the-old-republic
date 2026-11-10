// The wager popup before a match: the amount spoken as it changes, and Left
// and Right held down to step it faster.

#include "minigame_pazaak_internal.h"

#include "engine_panels.h"
#include "strings.h"

namespace acc::pazaak {

namespace {

// CSWGuiWagerPopup (a different panel from the board): current wager at
// +0xc94, maximum at +0xc98. The chain labels the
// less/more buttons but only re-reads on focus change, so we poll the amount
// and announce it when it changes (pure observation).
//
// KOTOR 2 = +0xE30 / +0xE34, and these are the least ambiguous constants in the
// whole batch: its HandleInputEvent @0x00887B10 clamps the decrement branch with
// `cmp [this+0xe30], 1` before `sub 1`, and the increment branch with
// `cmp [this+0xe30], [this+0xe34]` before `add 1`. Floor against current,
// current against ceiling — current is the low field in both games.
const size_t kWagerCurOffset = acc::off::Pick(0xc94, 0xE30);
const size_t kWagerMaxOffset = acc::off::Pick(0xc98, 0xE34);

void* g_wagerPanel = nullptr;
int   g_wagerLast  = -1;

// The engine's wager only ever changes by ±1 per input event (the less/more
// CSWGuiSpeedButtons just call HandleInputEvent 0x2f/0x30 — see decomp); a
// sighted player holds the mouse and the SpeedButton auto-repeats. We mask
// those buttons out of the chain (menus_chain RebindChain) and drive the wager
// with Left/Right instead, polled here so we can mirror the hold-to-repeat:
// a tap is one credit, holding accelerates toward the cap/floor. Per-step
// click feedback comes free (HandleInputEvent plays the wager-click sound);
// the spoken value is held back while it races and announced once on
// release.
int      g_wagerHeldDir   = 0;  // -1 decrease, +1 increase, 0 idle
unsigned g_wagerHoldStart = 0;  // GetTickCount at key-down
unsigned g_wagerNextStep  = 0;  // earliest tick for the next auto-repeat step
bool     g_wagerRepeating = false;  // crossed from single tap into auto-repeat

constexpr unsigned kWagerHoldDelayMs  = 350;   // grace before auto-repeat starts
constexpr unsigned kWagerRepeatSlowMs = 140;   // first repeat interval (~7/s)
constexpr unsigned kWagerRepeatFastMs = 30;    // terminal interval (~33/s)
constexpr unsigned kWagerRampMs       = 1200;  // accel span from slow to fast

void StepWager(void* fg, int dir) {
    DispatchWagerInput(fg, dir < 0 ? kWagerLessCode : kWagerMoreCode);
}

void SpeakWager(int cur, int max) {
    char msg[96];
    snprintf(msg, sizeof(msg), acc::strings::Get(acc::strings::Id::PazaakFmtWager), cur, max);
    Say(msg, true);
}

}  // namespace

// Run every tick while the wager popup may be foreground (from Tick). Owns the
// wager Left/Right stepper and announces the amount when it settles.
void ServiceWagerPopup(void* fg) {
    if (!fg || acc::engine::IdentifyPanel(fg) != acc::engine::PanelKind::PazaakWager) {
        g_wagerPanel     = nullptr;
        g_wagerLast      = -1;
        g_wagerHeldDir   = 0;
        g_wagerRepeating = false;
        return;
    }

    int cur = -1, max = -1;
    ReadIntAt(fg, kWagerCurOffset, &cur);
    ReadIntAt(fg, kWagerMaxOffset, &max);
    bool firstSight = (fg != g_wagerPanel);

    bool leftDown  = (GetAsyncKeyState(VK_LEFT)  & 0x8000) != 0;
    bool rightDown = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    int  dir = (leftDown == rightDown) ? 0 : (leftDown ? -1 : +1);
    unsigned now = GetTickCount();

    if (firstSight) {
        // Adopt the current key state without firing — a key held at popup-open
        // (left over from navigation) shouldn't auto-step.
        g_wagerHeldDir   = dir;
        g_wagerRepeating = false;
    } else if (dir != 0 && dir != g_wagerHeldDir) {
        // Rising edge / direction flip → one immediate step. The single-step
        // value is spoken by the change-announce below.
        StepWager(fg, dir);
        g_wagerHeldDir   = dir;
        g_wagerHoldStart = now;
        g_wagerNextStep  = now + kWagerHoldDelayMs;
        g_wagerRepeating = false;
    } else if (dir != 0) {  // same direction still held → auto-repeat
        if ((int)(now - g_wagerNextStep) >= 0) {
            StepWager(fg, dir);
            g_wagerRepeating = true;
            unsigned t = now - g_wagerHoldStart - kWagerHoldDelayMs;
            unsigned interval = (t >= kWagerRampMs)
                ? kWagerRepeatFastMs
                : kWagerRepeatSlowMs -
                      (kWagerRepeatSlowMs - kWagerRepeatFastMs) * t / kWagerRampMs;
            g_wagerNextStep = now + interval;
        }
    } else {  // no direction held
        if (g_wagerRepeating) {
            // Silent while it raced: the final value on release. Stamp
            // g_wagerLast so the change-announce stays quiet.
            SpeakWager(cur, max);
            g_wagerLast = cur;
        }
        g_wagerHeldDir   = 0;
        g_wagerRepeating = false;
    }

    // Value-change announce for taps (and any non-key change). Suppressed
    // mid-race; the release branch above owns that announcement.
    if (!firstSight && !g_wagerRepeating && cur != g_wagerLast) {
        SpeakWager(cur, max);
    }
    g_wagerPanel = fg;
    g_wagerLast  = cur;
}

}  // namespace acc::pazaak
