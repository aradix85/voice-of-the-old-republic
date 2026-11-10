// Gamepad input: loading XInput, the per-tick sample (triggers, sticks,
// buttons, which slot answers), pad presence, and whether the engine's own
// pad path is alive. See pad_input.h for why this is one module, and why
// KOTOR 1 and KOTOR 2 share every binding but not their event source.

#include "pad_input_internal.h"

#include "engine_game.h"
#include "engine_panels.h"
#include "engine_player.h"
#include "hotkeys.h"         // PadPress — reuse the keyboard actions' pollers
#include "log.h"
#include "pad_diag.h"        // the device census — what the machine actually
                             // has, as opposed to what we managed to bind
#include "peek_description.h" // OnShiftReleased — Y is the pad's peek modifier
#include "speech.h"           // Silence — the cancel a pad press has no
                             // keystroke to trigger

namespace acc::pad {

namespace {

// ---------------------------------------------------------------------------
// XInput, loaded at run time
// ---------------------------------------------------------------------------
// Minimal local mirrors of the two XInput structures. Declared here rather
// than including <XInput.h> so the patch keeps building against any SDK
// arrangement and so nothing can drag in a static import — the whole point of
// resolving this at run time is that a missing xinput DLL must degrade to "no
// triggers", never to a loader failure.
struct XiGamepad {
    WORD  wButtons;
    BYTE  bLeftTrigger;
    BYTE  bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
};
struct XiState {
    DWORD     dwPacketNumber;
    XiGamepad Gamepad;
};
typedef DWORD(WINAPI* PFN_XInputGetState)(DWORD dwUserIndex, XiState* pState);

// Firmer than XInput's own XINPUT_GAMEPAD_TRIGGER_THRESHOLD (30 of 255): the
// triggers here are a MODIFIER, and a modifier that engages on a resting
// finger would silently reroute the D-Pad. A deliberate pull clears this
// easily.
constexpr BYTE kTriggerHeld = 60;

// While no pad is found, retry at this cadence rather than every frame —
// XInputGetState on an empty slot is a comparatively expensive call.
constexpr ULONGLONG kScanIntervalMs = 2000;

// The idle-slot guard. More than one XInput device is ordinary on a handheld:
// the vendor layer (Armoury Crate on the ROG Ally, and its equivalents) can
// present a controller of its own beside the physical one, and from inside
// Tick() a slot bound to that is indistinguishable from everything working —
// it answers every poll, so the rescan below never fires, and the player's real
// pad is never read. XInput's packet number is the discriminator: it moves on
// every state change, so a bound slot whose packet is frozen this long while
// another connected slot's is moving is proof we bound the wrong device.
//
// Long enough that a player who has simply put the pad down does not trip it —
// and tripping it is harmless anyway, because the guard only ever switches TO a
// slot that demonstrably changed.
//
// The guard is NOT gated on how many devices answered when we bound, because
// that count goes stale the moment a device appears afterwards — which is
// exactly what a vendor layer switching modes mid-session does, and exactly the
// case the guard exists for. It discovers the alternates itself instead, and
// pays for that at the same cadence the unbound rescan already pays: four slot
// polls per interval, and only while the bound pad has been perfectly still for
// the idle window.
constexpr ULONGLONG kLatchIdleMs = 8000;
constexpr ULONGLONG kAltPollMs   = 2000;   // cadence while the guard is armed

HMODULE            g_xinputDll   = nullptr;
PFN_XInputGetState g_xinputGet   = nullptr;
const wchar_t*     g_xinputName  = nullptr;   // for the census line
bool               g_xinputTried = false;
int                g_padSlot     = -1;   // last slot that answered
ULONGLONG          g_lastScanMs  = 0;
bool               g_censusDone  = false;

// Idle-slot guard state. g_altSeen distinguishes "this alternate's packet
// differs from its baseline" from "we have no baseline for it yet" — without
// that, first sight of any alternate would read as movement.
DWORD     g_lastPacket     = 0;
ULONGLONG g_packetMovedMs  = 0;
ULONGLONG g_lastAltPollMs  = 0;
DWORD     g_altPacket[4]   = {};
bool      g_altSeen[4]     = {};

// Presence has two sources, and they answer with different lifetimes because
// the evidence behind them differs in kind.
//
//   XInput — polled every tick and rescanned every 2 s, so it is a LIVE fact.
//            It can say "a pad is plugged in right now", and — the part that
//            matters here — it can stop saying it when the player unplugs one.
//            Held as a timestamp with a forget window rather than as a bool, so
//            one failed read, or the gap between two throttled rescans, cannot
//            flap the answer.
//
//   Engine — KOTOR 2 only, and only ever an EDGE: a pad-shaped event arrived.
//   events   There is no "is it still there" question to ask of it, and the
//            engine enumerates game controllers exactly once at input init, so
//            a pad that announced itself this way stays the relevant one for
//            the session. Sticky by necessity, not by choice.
//
// Connected() is the OR of the two. It is the device signal the help and
// tutorial wording switches on (binding_hints.h), so an unplugged pad has to
// become visible, and a DirectInput-only pad on KOTOR 2, which XInput never
// reports, still has to count.
ULONGLONG g_xinputSeenMs = 0;   // last successful XInputGetState; 0 = never
bool      g_padEventSeen = false;

// Three rescans' worth. Long enough that a hiccup or a slot change is invisible
// to the wording switch, short enough that a player who unplugs the pad and
// reaches for the keyboard gets keyboard hints on their next tutorial popup.
constexpr ULONGLONG kPadForgetMs = 6000;

// ---------------------------------------------------------------------------
// Left-stick movement
// ---------------------------------------------------------------------------
// XInput's own left-stick dead zone is 7849 of 32767. This sits above it: the
// question here is "is the player commanding translation", not "has the stick
// left centre", and a resting thumb on a worn stick must not read as a walk.
constexpr int kStickDeadZone   = 12000;
constexpr int kStickDeadZoneSq = kStickDeadZone * kStickDeadZone;

bool  g_stickMoving = false;
float g_stickX      = 0.0f;   // normalised, +1 = right
float g_stickY      = 0.0f;   // normalised, +1 = up (XInput's own convention)

// The right stick, same convention. Rotate Camera on the shipped chart; on
// KOTOR 1 pad_movement holds the bound camera-turn key from it.
bool  g_rstickActive = false;
float g_rstickX      = 0.0f;
float g_rstickY      = 0.0f;

// One stick's sample: true past the dead zone, with the direction normalised
// into outX / outY; zero inside it. int arithmetic, not the SHORTs: the
// squared magnitude of two full-scale axes overflows 16 bits.
bool SampleStick(int x, int y, float& outX, float& outY) {
    const int magSq = x * x + y * y;
    if (magSq <= kStickDeadZoneSq) {
        outX = outY = 0.0f;
        return false;
    }
    const float mag = std::sqrt(static_cast<float>(magSq));
    outX = static_cast<float>(x) / mag;
    outY = static_cast<float>(y) / mag;
    return true;
}

// Everything the sample holds, read as released: the slot it came from is gone
// or no longer ours. Without this pad_movement would keep a synthesised walk
// key down, and the first read of the next slot would fire phantom edges
// against a stale baseline.
void ReleaseSampledState() {
    g_ltHeld = g_rtHeld = false;
    g_stickMoving  = false;
    g_stickX = g_stickY = 0.0f;
    g_rstickActive = false;
    g_rstickX = g_rstickY = 0.0f;
    g_btnNow  = 0;
    g_yHeld   = false;
}

void NoteEnginePadLive(const char* why) {
    if (g_enginePad == EnginePad::Live) return;
    const bool correcting = (g_enginePad == EnginePad::Dead);
    g_enginePad     = EnginePad::Live;
    g_probeCode     = 0;
    g_probeSlot     = -1;
    g_probeDeadline = 0;
    acclog::Write("Pad", "engine gamepad path is LIVE (%s)%s", why,
                  correcting ? " — correcting an earlier probe that read it as "
                               "dead; the engine owns its bindings again"
                             : " — the engine owns its own bindings");
}

void EnsureXInput() {
    if (g_xinputTried) return;
    g_xinputTried = true;
    // Newest first. 9_1_0 is the one that ships in-box on every supported
    // Windows, so it is the guaranteed fallback.
    static const wchar_t* const kDlls[] = {
        L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll",
    };
    for (const wchar_t* name : kDlls) {
        HMODULE h = LoadLibraryW(name);
        if (!h) continue;
        auto fn = reinterpret_cast<PFN_XInputGetState>(
            GetProcAddress(h, "XInputGetState"));
        if (fn) {
            g_xinputDll  = h;
            g_xinputGet  = fn;
            g_xinputName = name;   // static storage — the census prints it later
            acclog::Write("Pad", "XInput bound via %ls", name);
            return;
        }
        FreeLibrary(h);
    }
    acclog::Write("Pad", "no XInput DLL resolved — triggers unavailable "
                         "(D-Pad bindings still work)");
}

// ---------------------------------------------------------------------------
// The idle-slot guard — see kLatchIdleMs for what it is defending against.
// ---------------------------------------------------------------------------
// Called only while the bound slot has stood perfectly still for the idle
// window. Polls the slots we did NOT bind and switches to any that moved since its own baseline. Two samples
// are needed per alternate, which is why the first sight of one only records a
// baseline: a packet number read once says nothing about whether it is moving.
//
// Returns true when it switched, which the caller must treat as "this tick's
// sample belongs to a device we no longer read" and abandon.
bool AdoptLivelierSlot(ULONGLONG now) {
    if (g_lastAltPollMs != 0 && now - g_lastAltPollMs < kAltPollMs) return false;
    g_lastAltPollMs = now;

    for (DWORD i = 0; i < 4; ++i) {
        if (static_cast<int>(i) == g_padSlot) continue;
        XiState alt = {};
        if (g_xinputGet(i, &alt) != ERROR_SUCCESS) {
            g_altSeen[i] = false;   // gone; any baseline we had is meaningless
            continue;
        }
        if (!g_altSeen[i]) {
            g_altSeen[i]   = true;
            g_altPacket[i] = alt.dwPacketNumber;
            continue;
        }
        if (alt.dwPacketNumber == g_altPacket[i]) continue;

        // A moving packet number proves the alternate is CARRYING input. It
        // does not prove the input is coming from a different controller, and
        // that is the whole question here. Steam Input presents one physical
        // pad as two virtual XInput devices (both vendor Valve, both the same
        // product), feeding whichever of them it likes; each is idle exactly
        // when the other moves, so this test fires on every brief pause and
        // the guard would ping-pong, clearing the trigger and stick state
        // below mid-gesture each time.
        //
        // Same vendor AND same product means one device wearing two slots, and
        // there is nothing to win by moving between them: the presses are
        // identical. So the guard stands down. It keeps the case it was built
        // for — a handheld's vendor layer sitting in front of the built-in pad
        // is a DIFFERENT device with different ids, which is the only way the
        // two can be told apart from here.
        //
        // The cost is a player holding two controllers of the same model, who
        // now keeps whichever slot answered first. That is a worse outcome
        // than before for a rarer setup than the one this fixes, and it is
        // recoverable by unplugging the idle pad; a Steam layer is not
        // recoverable by anything the player can see.
        //
        // Ids unavailable (a runtime older than xinput1_4 has no
        // XInputGetCapabilitiesEx) means "cannot tell", and cannot-tell keeps
        // the original behaviour rather than inventing a match.
        unsigned boundVid = 0, boundPid = 0, altVid = 0, altPid = 0;
        if (acc::pad::diag::SlotVendorIds(g_xinputDll,
                                          static_cast<unsigned>(g_padSlot),
                                          &boundVid, &boundPid) &&
            acc::pad::diag::SlotVendorIds(g_xinputDll, i, &altVid, &altPid) &&
            boundVid == altVid && boundPid == altPid) {
            // Baseline moved forward so the next poll only re-enters here when
            // the mirror moves again, and Trace so a pad in constant use
            // collapses to one line with its own suppressed count.
            g_altPacket[i] = alt.dwPacketNumber;
            acclog::Trace("Pad",
                          "slot %u carries the same device as bound slot %d "
                          "(vid=0x%04X pid=0x%04X on both) — one pad presented "
                          "twice, most often Steam Input's virtual controller. "
                          "Staying on slot %d: a mirror has nothing the bound "
                          "slot does not, and switching would drop the held "
                          "trigger and stick state.",
                          i, g_padSlot, boundVid, boundPid, g_padSlot);
            continue;
        }

        acclog::Write("Pad",
                      "slot %d has not changed state in %llu ms while slot %u "
                      "is producing input — switching to slot %u. The first "
                      "slot that answered was not the pad the player is holding "
                      "(a second XInput device, most often a handheld's vendor "
                      "layer, was sitting in front of it).",
                      g_padSlot,
                      static_cast<unsigned long long>(now - g_packetMovedMs),
                      i, i);

        g_padSlot       = static_cast<int>(i);
        g_lastPacket    = alt.dwPacketNumber;
        g_packetMovedMs = now;

        ReleaseSampledState();
        g_ltChorded = g_rtChorded = false;
        for (int s = 0; s < 4; ++s) g_altSeen[s] = false;
        return true;
    }
    return false;
}

}  // namespace

// Only the two engine-native-discriminated event shapes are allowed to set
// this: a stick axis carrying val = +/-1 (the engine's own preamble keys on
// exactly that), and a D-Pad code (F9..F12, unbound in stock [Keymapping], and
// twin-checked on top). The face and shoulder codes are shared with C, D and
// the function keys — codes an ordinary keyboard-only KOTOR 2 session really
// does produce — and letting those claim a controller would put a Controller
// section in the F1 help of a player who has no controller.
void NoteSeen() {
    g_padEventSeen = true;
    // The same two discriminated shapes that prove a pad exists also prove the
    // ENGINE is delivering for it — which is the whole question the probe below
    // is asking.
    NoteEnginePadLive("a pad-shaped engine event arrived");
}

// ---------------------------------------------------------------------------
// Surfaces
// ---------------------------------------------------------------------------
// "World" means the pad is driving the game, not a panel: the player exists
// and no engine panel owns the foreground. Everything else — title screen,
// chargen, dialog, the in-game menu strip, any modal — is "menu", where the
// pad's job is simply to be the keyboard.
bool InWorldSurface() {
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return false;
    return !acc::engine::IsForegroundUiBlocking();
}

// The cancel a pad press has no keystroke to trigger.
//
// Every menu announcement in the mod speaks with interrupt=false — deliberately,
// because the screen reader itself cuts queued speech the moment the user
// presses a key, and letting it do that keeps multi-part announcements (title
// then focus) able to queue. A controller press produces no keystroke, so
// nothing cancels and the entries pile up: the user hears the previous row
// finish before the new one starts, worsening with every press.
//
// So issue the cancel ourselves, at the one place that knows the input came
// from a pad. This is exactly what the screen reader would have done, and it
// leaves every announce path unchanged — the alternative, flipping dozens of
// call sites to interrupt=true, would also change keyboard behaviour and break
// the announcements that intentionally queue.
void NoteNavPress() {
    acc::speech::Silence();
}

// ---------------------------------------------------------------------------
// Public surface
// ---------------------------------------------------------------------------

bool EnginePadDead() { return g_enginePad == EnginePad::Dead; }

bool LeftTriggerHeld()  { return g_ltHeld; }

bool RightTriggerHeld() { return g_rtHeld; }

bool Connected() {
    if (g_padEventSeen) return true;
    if (g_xinputSeenMs == 0) return false;
    return GetTickCount64() - g_xinputSeenMs <= kPadForgetMs;
}

void Tick() {
    // KOTOR 1's answer is not a probe result, it is a fact about the binary:
    // its raw-input layer creates a keyboard and a mouse and nothing else, so
    // the mod is the pad driver there from the first frame.
    if (g_enginePad == EnginePad::Unknown && acc::game::IsKotor1()) {
        g_enginePad = EnginePad::Dead;
        acclog::Write("Pad", "KOTOR 1 has no engine gamepad path — the mod is "
                             "the driver");
    }

    // Both games. KOTOR 2 reads the pad here only for the triggers, the stick
    // magnitude and pad presence — the engine delivers its presses. KOTOR 1
    // has no engine pad path at all, so this sample is the ONLY thing that
    // knows a controller was touched; PollButtons() and pad_movement read what
    // it leaves behind.
    EnsureXInput();

    // The device census, once per session. Deliberately ahead of the no-XInput
    // early-out: "the mod resolved no XInput runtime" and "this machine has no
    // controller attached" are different findings with the same symptom, and a
    // tester's log has to tell them apart even when nothing works at all. The
    // census also reaches DirectInput, which is the only half that can name a
    // device — and on KOTOR 2 it is the same enumeration the engine itself ran
    // at input init.
    if (!g_censusDone) {
        g_censusDone = true;
        acc::pad::diag::LogDeviceCensus(g_xinputDll, g_xinputName,
                                        "session start");
    }

    if (!g_xinputGet) return;

    const ULONGLONG now = GetTickCount64();

    // Slot search, throttled. The first slot that ANSWERS is not automatically
    // the pad the player is holding — see kLatchIdleMs — so prefer one whose
    // capabilities report a gamepad, fall back to the first that answers, and
    // leave the rest to the idle-slot guard.
    if (g_padSlot < 0) {
        if (g_lastScanMs != 0 && now - g_lastScanMs < kScanIntervalMs) return;
        g_lastScanMs = now;

        // A pad plugged in mid-session changes the device picture, and the
        // census is the only line that says WHICH device appeared. Costs
        // nothing while nothing changes: the fingerprint it compares is
        // XInput-only, and the DirectInput walk runs only when it prints.
        acc::pad::diag::LogDeviceCensusIfChanged(g_xinputDll, g_xinputName,
                                                 "the device picture changed");

        int firstAnswering = -1;
        int firstGamepad   = -1;
        int connected      = 0;
        for (DWORD i = 0; i < 4; ++i) {
            XiState probe = {};
            if (g_xinputGet(i, &probe) != ERROR_SUCCESS) continue;
            ++connected;
            if (firstAnswering < 0) firstAnswering = static_cast<int>(i);
            if (firstGamepad < 0 &&
                acc::pad::diag::SlotSubType(g_xinputDll, i) ==
                    acc::pad::diag::kSubTypeGamepad) {
                firstGamepad = static_cast<int>(i);
            }
        }
        if (firstAnswering < 0) return;

        g_padSlot   = (firstGamepad >= 0) ? firstGamepad : firstAnswering;
        acclog::Write("Pad",
                      "XInput controller found in slot %d — %d device(s) "
                      "connected, chosen because %s",
                      g_padSlot, connected,
                      (firstGamepad >= 0)
                          ? "its capabilities report a gamepad"
                          : "it is the first slot that answered");

        // Baseline for the idle-slot guard. Started here so it never inherits
        // the clock of whatever was bound before.
        g_lastPacket    = 0;
        g_packetMovedMs = now;
        g_lastAltPollMs = 0;
        for (int s = 0; s < 4; ++s) g_altSeen[s] = false;
    }

    XiState st = {};
    if (g_xinputGet(static_cast<DWORD>(g_padSlot), &st) != ERROR_SUCCESS) {
        acclog::Write("Pad", "XInput slot %d stopped answering — rescanning",
                      g_padSlot);
        g_padSlot = -1;
        ReleaseSampledState();
        return;
    }

    // The live half of Connected(): a slot that answered this tick. Stamped
    // here rather than at discovery so an unplug stops refreshing it.
    g_xinputSeenMs = now;

    // The idle-slot guard. A bound slot that answers but never CHANGES is the
    // fingerprint of a device nobody is holding — which is either a pad the
    // player has put down, or the wrong device entirely. Only the alternates
    // can tell those apart, so the decision is deferred to AdoptLivelierSlot.
    if (st.dwPacketNumber != g_lastPacket) {
        g_lastPacket    = st.dwPacketNumber;
        g_packetMovedMs = now;
    } else if (g_packetMovedMs != 0 &&
               now - g_packetMovedMs >= kLatchIdleMs) {
        // The sample in hand came from the slot we just abandoned. Drop the
        // rest of this tick rather than feed it downstream — the cleared state
        // the switch left behind is the correct baseline for the next one.
        if (AdoptLivelierSlot(now)) return;
    }

    g_btnNow = st.Gamepad.wButtons;

    // Y as the peek modifier. Its RELEASE resets the description-block cursor,
    // exactly as the keyboard's Shift release does (cycle_input's shift
    // tracker) — without that the next peek would resume mid-description
    // instead of at block 0.
    const bool y = (st.Gamepad.wButtons & kBtnY) != 0;
    if (g_yHeld && !y) acc::peek::OnShiftReleased();
    g_yHeld = y;

    const bool lt = st.Gamepad.bLeftTrigger  >= kTriggerHeld;
    const bool rt = st.Gamepad.bRightTrigger >= kTriggerHeld;
    if (lt != g_ltHeld || rt != g_rtHeld) {
        acclog::Write("Pad", "trigger state LT=%d RT=%d (raw %u/%u)",
                      lt ? 1 : 0, rt ? 1 : 0,
                      static_cast<unsigned>(st.Gamepad.bLeftTrigger),
                      static_cast<unsigned>(st.Gamepad.bRightTrigger));
    }
    const bool ltPressed  =  lt && !g_ltHeld;
    const bool ltReleased = !lt &&  g_ltHeld;
    const bool rtPressed  =  rt && !g_rtHeld;
    const bool rtReleased = !rt &&  g_rtHeld;
    g_ltHeld = lt;
    g_rtHeld = rt;

    if (ltPressed) g_ltChorded = false;
    if (rtPressed) g_rtChorded = false;

    // XInput answers whether or not KOTOR has focus, so the trigger ACTIONS
    // are gated on the game being foreground: a trigger pulled while the
    // player is alt-tabbed must not speak over whatever they switched to, nor
    // silently open a menu under them. The edge state above is tracked
    // regardless, so a chord begun in the background still cancels its
    // trigger's solo job instead of firing it on the way back — and the stick
    // sampling below stays outside the gate, because its consumers want a
    // truthful "not moving" while the game is away, not a stale "moving".
    //
    // What the edges MEAN is decided here; when it is acted on is not. The
    // three trigger jobs are only recorded as pending and are drained by
    // PollTriggers() in the input-poll slot — LT opens the unified action menu,
    // and the menu's hard rule is that it may not be populated from OnUpdate,
    // which is where this function runs.
    if (acc::hotkeys::IsForegroundGame()) {
        // Both triggers — the context help ("what do the keys do on THIS
        // screen"). Claimed on whichever press completes the pair, and marks
        // both holds chorded so neither release also does its solo job.
        if ((ltPressed && rt) || (rtPressed && lt)) {
            g_ltChorded = g_rtChorded = true;
            g_pendingHelp = true;
        }

        // LT alone — open or close the action menu.
        if (ltReleased && !g_ltChorded) g_pendingLtSolo = true;

        // RT alone — the exact keyboard AltGr binding: announce the facing.
        if (rtReleased && !g_rtChorded) g_pendingRtSolo = true;
    }

    // Left stick = Move on the shipped binding chart, so its magnitude is the
    // "player is commanding translation" signal the drive-loop suppression and
    // the autowalk cancel both want.
    g_stickMoving = SampleStick(st.Gamepad.sThumbLX, st.Gamepad.sThumbLY,
                                g_stickX, g_stickY);
    acclog::Edge("Pad.stick", g_stickMoving ? 1 : 0,
                 "left stick moving=%d (x=%d y=%d)", g_stickMoving ? 1 : 0,
                 st.Gamepad.sThumbLX, st.Gamepad.sThumbLY);

    // Right stick. Unused on KOTOR 2 (the engine rotates its own camera from
    // it); on KOTOR 1 it is what pad_movement turns the camera with.
    g_rstickActive = SampleStick(st.Gamepad.sThumbRX, st.Gamepad.sThumbRY,
                                 g_rstickX, g_rstickY);
}

bool StickMoving() { return g_stickMoving; }

bool InWorld() { return InWorldSurface(); }

bool StickVector(float& outX, float& outY) {
    if (!g_stickMoving) return false;
    outX = g_stickX;
    outY = g_stickY;
    return true;
}

bool RightStickVector(float& outX, float& outY) {
    if (!g_rstickActive) return false;
    outX = g_rstickX;
    outY = g_rstickY;
    return true;
}

}  // namespace acc::pad
