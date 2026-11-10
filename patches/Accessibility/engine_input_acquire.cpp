// DirectInput device state: the change log, and the deferred reacquire and release of the keyboard
// and mouse.

#include "engine_input_internal.h"

#include "log.h"
#include "engine_offsets_select.h"  // acc::off — the CExoRawInputInternal
                                    // device fields moved by 4 bytes in K2
#include "speech.h"
#include "strings.h"

namespace acc::engine {

namespace {

// CExoInput::SetActive(this, int active) @0x005df540. Sets the facade +
// CExoInputInternal active flags and forwards to
// CExoRawInputInternal::SetActive, which Acquire()s the DirectInput
// keyboard/mouse/joystick devices on the 0->1 transition.
//
// KOTOR 2 @0x0072fb20 — the same three-layer chain, decompiled 2026-08-13:
// 0x0072fb20 forwards this->internal to CExoInputInternal::SetActive
// (0x0072df60), which stores the flag and forwards to
// CExoRawInputInternal::SetActive (0x00733090). That last one confirms the
// design this file depends on: it early-outs when the requested state already
// matches, then calls IDirectInputDevice8::Acquire (vtable +0x1c) or
// ::Unacquire (+0x20) on the keyboard, the mouse and every joystick. So the
// 0->1 edge ForceReacquireInput drives is required on KOTOR 2 for the same
// reason it is on KOTOR 1.
const uintptr_t kAddrCExoInputSetActive =
    acc::addr::Pick(0x005df540, 0x0072fb20);

typedef void(__thiscall* PFN_CExoInputSetActive)(void* this_, int active);

// CExoRawInputInternal fields live in engine_input.h (raw_input::kOff*) —
// shared with the keyboard stream re-sync, which walks the same object.
// CExoInput::internal, then CExoInputInternal::raw_input — the walk from the
// ExoInput global to the object that owns the DirectInput devices. KOTOR 2's
// raw_input offset is read off CExoInputInternal::SetActive, which tests that
// field for NULL before forwarding.
const size_t kOffInputInternal = acc::off::Same(0x04);
const size_t kOffInternalRaw   = acc::off::Pick(0x140, 0x1a0);

// Everything the probe reports, in one POD so the SEH reader can fill it and
// the (SEH-free) caller can act on it. Split for C2712: the caller speaks and
// formats, both of which want objects.
struct DiSnapshot {
    void*        raw;
    unsigned int active;
    void*        iface;
    void*        keyboard;
    void*        mouse;
};

DiSnapshot g_lastDi{};
bool       g_haveDi = false;
bool       g_keyboardLostWarned = false;

// Pending input-acquisition change, set from focus-event wndprocs and drained
// on the next clean tick. 0 = nothing, 1 = acquire (gained foreground),
// 2 = release (lost foreground). Last writer wins, so a rapid loss->gain (or
// gain->loss) flip collapses to its final state at drain — input ends up
// matching the foreground state that actually settled.
std::atomic<int> g_pendingInputState{0};

bool ReadDiSnapshot(DiSnapshot* out) {
    __try {
        if (!acc::addr::Ok(kAddrExoInputGlobal)) return false;
        void* exoInput = *reinterpret_cast<void**>(kAddrExoInputGlobal);
        if (!exoInput) return false;
        void* internal = *reinterpret_cast<void**>(
            static_cast<uint8_t*>(exoInput) + kOffInputInternal);
        if (!internal) return false;
        void* raw = *reinterpret_cast<void**>(
            static_cast<uint8_t*>(internal) + kOffInternalRaw);
        if (!raw) return false;

        auto* base = static_cast<uint8_t*>(raw);
        out->raw      = raw;
        out->active   = *reinterpret_cast<unsigned int*>(base + raw_input::kOffActive);
        out->iface    = *reinterpret_cast<void**>(base + raw_input::kOffInterface);
        out->keyboard = *reinterpret_cast<void**>(base + raw_input::kOffKeyboard);
        out->mouse    = *reinterpret_cast<void**>(base + raw_input::kOffMouse);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

void LogDirectInputStateIfChanged() {
    DiSnapshot now{};
    if (!ReadDiSnapshot(&now)) return;

    if (g_haveDi && now.raw == g_lastDi.raw && now.active == g_lastDi.active &&
        now.iface == g_lastDi.iface && now.keyboard == g_lastDi.keyboard &&
        now.mouse == g_lastDi.mouse) {
        return;
    }

    // A keyboard device that goes from present to NULL is the failure this
    // whole file is about. NULL-from-the-start is a different (and normal)
    // state — the engine has simply not built its devices yet — so it is
    // logged but never spoken.
    const bool keyboardLost =
        g_haveDi && g_lastDi.keyboard != nullptr && now.keyboard == nullptr;

    g_lastDi = now;
    g_haveDi = true;

    acclog::Write("EngineInput",
        "DirectInput state: raw=%p active=%u interface=%p keyboard=%p "
        "mouse=%p%s",
        now.raw, now.active, now.iface, now.keyboard, now.mouse,
        keyboardLost ? " [KEYBOARD DEVICE RELEASED]" : "");

    if (keyboardLost && !g_keyboardLostWarned) {
        g_keyboardLostWarned = true;
        acclog::Write("EngineInput",
            "DirectInput: the engine released its keyboard device — no key "
            "press can reach the game from here, and SetActive cannot "
            "re-acquire a device that no longer exists; warning the player");
        acc::speech::Speak(
            acc::strings::Get(acc::strings::Id::InputKeyboardLost), /*interrupt=*/true);
    }
}

bool ForceReacquireInput() {
    __try {
        void* exoInput = *reinterpret_cast<void**>(kAddrExoInputGlobal);
        if (!exoInput) {
            acclog::Write("EngineInput",
                "ForceReacquireInput: ExoInput global is null; skipped");
            return false;
        }
        auto setActive = reinterpret_cast<PFN_CExoInputSetActive>(
            kAddrCExoInputSetActive);
        // Drive a real 0->1 edge. SetActive(1) alone no-ops when the active
        // flag is already (stale) 1; the leading SetActive(0) forces the
        // transition so the engine re-Acquires the DirectInput devices.
        setActive(exoInput, 0);
        setActive(exoInput, 1);
        acclog::Write("EngineInput",
            "ForceReacquireInput: CExoInput::SetActive(%p, 0)->(1) cycle "
            "dispatched (post-load DirectInput re-acquire)", exoInput);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("EngineInput",
            "ForceReacquireInput: exception during SetActive cycle; skipped");
        return false;
    }
}

bool ReleaseInput() {
    __try {
        void* exoInput = *reinterpret_cast<void**>(kAddrExoInputGlobal);
        if (!exoInput) {
            acclog::Write("EngineInput",
                "ReleaseInput: ExoInput global is null; skipped");
            return false;
        }
        auto setActive = reinterpret_cast<PFN_CExoInputSetActive>(
            kAddrCExoInputSetActive);
        // SetActive(0) unacquires the DirectInput devices. KOTOR acquires its
        // keyboard at background level (it keeps reading even when another
        // window is foreground — harmless in fullscreen, how the game shipped).
        // Windowed, that means menu nav keys bleed into the game while the user
        // is in another window (e.g. a screen reader). Releasing on focus-loss
        // makes input mirror foreground: the game holds the keyboard only while
        // it owns the foreground. The regain edge re-Acquires via
        // ForceReacquireInput.
        setActive(exoInput, 0);
        acclog::Write("EngineInput",
            "ReleaseInput: CExoInput::SetActive(%p, 0) dispatched "
            "(lost foreground — unacquire so keys don't bleed into the "
            "background game)", exoInput);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("EngineInput",
            "ReleaseInput: exception during SetActive; skipped");
        return false;
    }
}

void RequestInputReacquire() {
    g_pendingInputState.store(1, std::memory_order_relaxed);
}

void RequestInputRelease() {
    g_pendingInputState.store(2, std::memory_order_relaxed);
}

void DrainPendingReacquire() {
    int want = g_pendingInputState.exchange(0, std::memory_order_relaxed);
    if (want == 1) {
        acclog::Write("EngineInput",
            "DrainPendingReacquire: focus gain flagged; forcing DirectInput "
            "re-grab");
        ForceReacquireInput();
    } else if (want == 2) {
        acclog::Write("EngineInput",
            "DrainPendingReacquire: focus loss flagged; releasing DirectInput");
        ReleaseInput();
    }
}

}  // namespace acc::engine
