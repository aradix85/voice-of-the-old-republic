// DirectInput device state: the change log, and the deferred reacquire and
// release of the keyboard and mouse.

#include "engine_input_internal.h"

#include "engine_offsets_select.h"  // acc::off: the device fields moved 4 bytes in K2
#include "log.h"
#include "speech.h"
#include "strings.h"

#include <windows.h>

#include <atomic>

namespace acc::engine {

namespace {

// CExoInput::SetActive(this, int active). Both games forward through
// CExoInputInternal::SetActive to CExoRawInputInternal::SetActive (K2
// 0x0072df60, 0x00733090), which early-outs when the state already matches and
// otherwise Acquires or Unacquires the keyboard, the mouse and every joystick.
// Hence the 0->1 edge in ForceReacquireInput.
const uintptr_t kAddrCExoInputSetActive = acc::addr::Pick(0x005df540, 0x0072fb20);
typedef void(__thiscall* PFN_CExoInputSetActive)(void* this_, int active);

// CExoInput::internal, then CExoInputInternal::raw_input: the walk to the
// object that owns the devices (whose fields are raw_input::kOff* in
// engine_input.h). K2's raw_input offset is read off CExoInputInternal::
// SetActive, which tests that field for NULL before forwarding.
const size_t kOffInputInternal = acc::off::Same(0x04);
const size_t kOffInternalRaw   = acc::off::Pick(0x140, 0x1a0);

// Filled under SEH, acted on outside it (speech and formatting need objects,
// which C2712 keeps out of __try functions).
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

// Set by the focus-event wndprocs, drained on the next clean tick: 0 nothing,
// 1 acquire (foreground gained), 2 release (foreground lost). The last writer
// wins, so a quick flip settles on the final foreground state.
std::atomic<int> g_pendingInputState{0};

constexpr int kReacquireCycle[] = {0, 1};
constexpr int kRelease[]        = {0};

bool ReadDiSnapshot(DiSnapshot* out) {
    __try {
        void* exoInput = ExoInput();
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

bool SameDi(const DiSnapshot& a, const DiSnapshot& b) {
    return a.raw == b.raw && a.active == b.active && a.iface == b.iface &&
           a.keyboard == b.keyboard && a.mouse == b.mouse;
}

// SetActive with each of `states` in turn; *exoOut gets the facade. False when
// there is no facade or the call faulted.
bool CallSetActive(const int* states, int count, void** exoOut) {
    __try {
        void* exoInput = ExoInput();
        *exoOut = exoInput;
        if (!exoInput || !acc::addr::Ok(kAddrCExoInputSetActive)) return false;
        auto setActive = reinterpret_cast<PFN_CExoInputSetActive>(kAddrCExoInputSetActive);
        for (int i = 0; i < count; ++i) setActive(exoInput, states[i]);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

void LogDirectInputStateIfChanged() {
    DiSnapshot now{};
    if (!ReadDiSnapshot(&now)) return;
    if (g_haveDi && SameDi(now, g_lastDi)) return;

    // A keyboard device going from present to NULL is the failure to warn
    // about; NULL from the start only means the engine has not built it yet.
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
    // SetActive(1) alone no-ops while the flag is a stale 1; the leading 0
    // forces the edge on which the engine re-Acquires its devices.
    void* exoInput = nullptr;
    if (!CallSetActive(kReacquireCycle, 2, &exoInput)) {
        acclog::Write("EngineInput", "ForceReacquireInput: SetActive cycle failed "
                      "(ExoInput=%p); skipped", exoInput);
        return false;
    }
    acclog::Write("EngineInput",
        "ForceReacquireInput: CExoInput::SetActive(%p, 0)->(1) cycle "
        "dispatched (post-load DirectInput re-acquire)", exoInput);
    return true;
}

bool ReleaseInput() {
    // KOTOR acquires its keyboard at background level, so a windowed game
    // would keep reading keys typed in another window (a screen reader, say).
    // Unacquiring on focus loss makes input follow the foreground.
    void* exoInput = nullptr;
    if (!CallSetActive(kRelease, 1, &exoInput)) {
        acclog::Write("EngineInput", "ReleaseInput: SetActive failed (ExoInput=%p); "
                      "skipped", exoInput);
        return false;
    }
    acclog::Write("EngineInput",
        "ReleaseInput: CExoInput::SetActive(%p, 0) dispatched "
        "(lost foreground — unacquire so keys don't bleed into the "
        "background game)", exoInput);
    return true;
}

void RequestInputReacquire() {
    g_pendingInputState.store(1, std::memory_order_relaxed);
}

void RequestInputRelease() {
    g_pendingInputState.store(2, std::memory_order_relaxed);
}

void DrainPendingReacquire() {
    const int want = g_pendingInputState.exchange(0, std::memory_order_relaxed);
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
