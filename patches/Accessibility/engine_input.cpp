// The CExoInput facade: the InputIndices names for the logs, the manager's
// code translation, the camera turn axis and stick, and the time of the last
// engine key event. The mouse guard and the DirectInput acquire live in
// engine_input_mouseguard.cpp and engine_input_acquire.cpp.

#include "engine_input_internal.h"

#include <windows.h>

#include <atomic>

namespace acc::engine {

namespace {

// Engine global holding the CExoInput facade pointer (SYMBOL ExoInput in
// Lane's DB for K1). K2 @0x00a1b48c, by two witnesses:
//   * the constructor at 0x0072f760 writes the CExoInput RTTI vtable
//     (0x0099cab8) into the new object, and 0x00409c7e stores that object to
//     0x00a1b48c;
//   * the engine's global-pointer cluster keeps KOTOR 1's relative layout
//     (ExoResMan/GuiMan/Aurora/AppMan/VM/Tlk/Rules deltas all match), and
//     KOTOR 1's ExoInput is likewise the dword below ExoResMan.
const uintptr_t kAddrExoInputGlobal = acc::addr::PickGlobal(0x007a39e4, 0x00a1b48c);

// CExoInput::PollInput(this, code, inputClass) -> float. The camera's turn
// source: UpdateCamera reads it as PollInput(ExoInput, 0x11c, 0) on both games
// (K1 @0x005f5ee6: MOV ECX,[0x007a39e4]; PUSH 0; PUSH 0x11c; CALL 0x005df4d0.
// K2 @0x0078c48f: MOV ECX,[0x00a1b48c]; PUSH 0; PUSH 0x11c; CALL 0x0072f9d0 -
// a thiscall forwarder to CExoInputInternal::PollInput @0x0072c320 via
// this+4; both RET 8, float in ST0). Digital bindings answer the GetEvents
// level flag scaled, so this IS the engine's own view of "turn key held".
const uintptr_t kAddrCExoInputPollInput = acc::addr::Pick(0x005df4d0, 0x0072f9d0);
constexpr int kInputCodeTurnAxis = 0x11c;
typedef float(__thiscall* PFN_CExoInputPollInput)(void* this_, int code, int inputClass);

// KOTOR 2 only: CExoInput facade @0x0072fde0 -> CExoInputInternal
// ::ReadJoystickAxes(1, &x, &y) @0x0072ed70, the native right-stick pair
// UpdateCamera feeds to AcclTurnCamera ahead of the edge test. With no
// controller enumerated both outputs stay 0. KOTOR 1 has no joystick path.
const uintptr_t kAddrCExoInputReadStickAxes = acc::addr::Kotor2Only(0x0072fde0);
typedef void(__thiscall* PFN_CExoInputReadStickAxes)(void* this_, float* x, float* y);

// The last engine-delivered key press, GetTickCount64 (0 = none yet). Written
// by the input hooks, read by the tick; atomic so the 64-bit read cannot tear.
std::atomic<unsigned long long> g_lastEngineKeyMs{0};

}  // namespace

void* ExoInput() {
    return *reinterpret_cast<void**>(kAddrExoInputGlobal);
}

// The InputIndices enum (Lane's SARIF, /KotOR Types/Enums/InputIndices):
// index = value; -1 is INPUTDEVICE_NONE.
const char* InputIndexName(int code) {
    static const char* const k_names[] = {
        "MOUSE_BUTTON0", "MOUSE_BUTTON1", "MOUSE_BUTTON2",
        "MOUSE_XAXIS", "MOUSE_YAXIS", "MOUSE_ZAXIS",
        "KEYBOARD_RETURN",
        "KEYBOARD_LEFT_ARROW", "KEYBOARD_RIGHT_ARROW",
        "KEYBOARD_UP_ARROW", "KEYBOARD_DOWN_ARROW",
        "KEYBOARD_NUMPAD1", "KEYBOARD_NUMPAD2", "KEYBOARD_NUMPAD3",
        "KEYBOARD_NUMPAD4", "KEYBOARD_NUMPAD5", "KEYBOARD_NUMPAD6",
        "KEYBOARD_NUMPAD7", "KEYBOARD_NUMPAD8", "KEYBOARD_NUMPAD9",
        "KEYBOARD_NUMPAD0",
        "KEYBOARD_NUMPADDECIMAL", "KEYBOARD_NUMPADMINUS", "KEYBOARD_NUMPADPLUS",
        "KEYBOARD_LEFTSHIFT", "KEYBOARD_RIGHTSHIFT",
        "KEYBOARD_LEFTALT", "KEYBOARD_RIGHTALT",
        "KEYBOARD_LEFTCTRL", "KEYBOARD_RIGHTCTRL",
        "KEYBOARD_TAB", "KEYBOARD_ESC",
        "KEYBOARD_HOME", "KEYBOARD_END",
        "KEYBOARD_PAGEUP", "KEYBOARD_PAGEDOWN",
        "KEYBOARD_INSERT", "KEYBOARD_DELETE",
        "KEYBOARD_PRINTSCREEN",
        "KEYBOARD_F1", "KEYBOARD_F2", "KEYBOARD_F3", "KEYBOARD_F4",
        "KEYBOARD_F5", "KEYBOARD_F6", "KEYBOARD_F7", "KEYBOARD_F8",
        "KEYBOARD_F9", "KEYBOARD_F10", "KEYBOARD_F11", "KEYBOARD_F12",
        "KEYBOARD_A", "KEYBOARD_B", "KEYBOARD_C", "KEYBOARD_D",
        "KEYBOARD_E", "KEYBOARD_F", "KEYBOARD_G", "KEYBOARD_H",
        "KEYBOARD_I", "KEYBOARD_J", "KEYBOARD_K", "KEYBOARD_L",
        "KEYBOARD_M", "KEYBOARD_N", "KEYBOARD_O", "KEYBOARD_P",
        "KEYBOARD_Q", "KEYBOARD_R", "KEYBOARD_S", "KEYBOARD_T",
        "KEYBOARD_U", "KEYBOARD_V", "KEYBOARD_W", "KEYBOARD_X",
        "KEYBOARD_Y", "KEYBOARD_Z",
        "KEYBOARD_1", "KEYBOARD_2", "KEYBOARD_3", "KEYBOARD_4",
        "KEYBOARD_5", "KEYBOARD_6", "KEYBOARD_7", "KEYBOARD_8",
        "KEYBOARD_9", "KEYBOARD_0",
        "KEYBOARD_SPACE", "KEYBOARD_NUMPADENTER",
        "KEYBOARD_CAPSLOCK", "KEYBOARD_PAUSE",
        "KEYBOARD_F13", "KEYBOARD_F14", "KEYBOARD_F15",
        "KEYBOARD_MINUS", "KEYBOARD_EQUALS", "KEYBOARD_BACK",
        "KEYBOARD_LBRACKET", "KEYBOARD_RBRACKET",
        "KEYBOARD_SEMICOLON", "KEYBOARD_APOSTROPHE",
        "KEYBOARD_GRAVE", "KEYBOARD_BACKSLASH",
        "KEYBOARD_COMMA", "KEYBOARD_PERIOD", "KEYBOARD_SLASH",
        "KEYBOARD_MULTIPLY", "KEYBOARD_NUMPADCOMMA", "KEYBOARD_DIVIDE",
        "KEYBOARD_OEM_102",
        "JOYSTICK_XAXIS", "JOYSTICK_YAXIS", "JOYSTICK_HAT",
        "JOYSTICK_SLIDER0", "JOYSTICK_SLIDER1", "JOYSTICK_SLIDER2",
        "JOYSTICK_BUTTON0", "JOYSTICK_BUTTON1", "JOYSTICK_BUTTON2",
        "JOYSTICK_BUTTON3", "JOYSTICK_BUTTON4", "JOYSTICK_BUTTON5",
        "JOYSTICK_BUTTON6", "JOYSTICK_BUTTON7", "JOYSTICK_BUTTON8",
        "JOYSTICK_BUTTON9", "JOYSTICK_BUTTON10", "JOYSTICK_BUTTON11",
        "JOYSTICK_BUTTON12", "JOYSTICK_BUTTON13", "JOYSTICK_BUTTON14",
    };
    if (code == -1) return "INPUTDEVICE_NONE";
    if (code >= 0 && code < static_cast<int>(sizeof(k_names) / sizeof(k_names[0]))) {
        return k_names[code];
    }
    // A logical action the manager passes through untranslated.
    if (code == 0xCE) return "LOGICAL_TAB";
    return "?";
}

// CSWGuiManager::HandleInputEvent (0x0040c8e0) translates the engine's logical
// navigation codes to InputIndices before the panel sees them:
//
//   0xb4 (180), 0xdf (223)   -> 0x28 = KEYBOARD_F2  (cancel / back)
//   0xb5 (181), 0xbb (187)   -> 0x27 = KEYBOARD_F1  (confirm / activate)
//   0xb6 (182)               -> 0x3d
//   0xb7 (183)               -> 0x3e
//   0xb8 (184)               -> 0x3f
//   0xb9 (185)               -> 0x40
//
// Other codes pass through. 0x3d-0x40 are whatever the user's [Keymapping]
// binds to the navigation actions, so they get no name.
int ManagerTranslateCode(int code) {
    switch (code) {
    case 0xb4: case 0xdf: return 0x28;
    case 0xb5: case 0xbb: return 0x27;
    case 0xb6:            return 0x3d;
    case 0xb7:            return 0x3e;
    case 0xb8:            return 0x3f;
    case 0xb9:            return 0x40;
    default:              return code;
    }
}

bool PollTurnAxis(float* out) {
    void* exoInput = ExoInput();
    if (!out || !exoInput) return false;
    __try {
        auto poll = reinterpret_cast<PFN_CExoInputPollInput>(kAddrCExoInputPollInput);
        *out = poll(exoInput, kInputCodeTurnAxis, 0);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadNativeStickAxes(float* x, float* y) {
    void* exoInput = ExoInput();
    if (!x || !y || !exoInput || !acc::addr::Ok(kAddrCExoInputReadStickAxes)) return false;
    __try {
        auto read = reinterpret_cast<PFN_CExoInputReadStickAxes>(kAddrCExoInputReadStickAxes);
        *x = 0.0f;
        *y = 0.0f;
        read(exoInput, x, y);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void NoteEngineKeyEvent() {
    g_lastEngineKeyMs.store(GetTickCount64(), std::memory_order_relaxed);
}

unsigned long long LastEngineKeyEventMs() {
    return g_lastEngineKeyMs.load(std::memory_order_relaxed);
}

}  // namespace acc::engine
