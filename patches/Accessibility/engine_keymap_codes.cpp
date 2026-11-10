// The engine's key codes as Windows sees them: InputIndices values to
// virtual keys and to DirectInput scancodes. Engine-free.

#include "engine_keymap_codes.h"

#include <windows.h>  // VK_* constants only

#include "engine_keymap.h"

namespace acc::engine_keymap {

// Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY. Both are
// physical-key identifiers, so this is layout-independent — unlike routing
// through InputIndexToVk + the active layout, which on QWERTZ swaps Y/Z (turn
// bound to physical Z came out as physical Y and drove nothing). DirectInput,
// which the engine polls for the turn axis, wants the physical scancode. Covers
// the keys a turn bind realistically uses (letters, digits, space); returns 0
// for anything else so the caller can fall back.
int InputIndexToScancode(int ii) {
    // A..Z (InputIndex 0x33..0x4c) → DIK by physical position (not alphabetical).
    static const unsigned char kLetterDik[26] = {
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26,
        0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D,
        0x15, 0x2C};
    if (ii >= 0x33 && ii <= 0x4c) return kLetterDik[ii - 0x33];  // A..Z
    if (ii >= 0x4d && ii <= 0x55) return 0x02 + (ii - 0x4d);     // 1..9
    if (ii == 0x56)               return 0x0B;                   // 0
    if (ii == 0x57)               return 0x39;                   // Space
    // The non-letter keys KOTOR's own default binds land on: Tab (ChangeChar),
    // Escape (GUI) and the function row (Quicksave = F4, Quickload = F5).
    if (ii == 0x1e)               return 0x0F;                   // Tab
    if (ii == 0x1f)               return 0x01;                   // Escape
    if (ii >= 0x27 && ii <= 0x30) return 0x3B + (ii - 0x27);     // F1..F10
    if (ii == 0x31)               return 0x57;                   // F11
    if (ii == 0x32)               return 0x58;                   // F12
    // CAPSLOCK (InputIndex 89) is deliberately NOT mapped, even though KOTOR's
    // default Freelook bind uses it. Everything that resolves a scancode here
    // does so in order to INJECT it, and injecting Caps Lock toggles the OS
    // lock state and fires the screen reader's modifier key. A caller that
    // gets 0 declines, which is the right answer for that key.
    return 0;
}

int InputIndexToVk(int ii) {
    // KOTOR's keymap stores an `InputIndices` enum value (NOT a DIK scancode):
    // CExoInput::GetLastCapturedKeyboardKey returns one, the keymap row's
    // key_code holds one, and swkotor.ini [Keymapping] writes it in decimal.
    // Values from the InputIndices enum (k1_win_gog_swkotor.exe.xml). We map to
    // the Win32 VK the mod's GetAsyncKeyState bindings use. (Direct VK mapping:
    // exact on US layouts and for every physical key the mod actually binds;
    // the only ambiguity is the QWERTZ Y/Z swap, which no mod hotkey touches.)
    if (ii >= 0x33 && ii <= 0x4c) return 'A' + (ii - 0x33);  // KEYBOARD_A..Z
    if (ii >= 0x4d && ii <= 0x55) return '1' + (ii - 0x4d);  // KEYBOARD_1..9
    if (ii == 0x56)               return '0';                // KEYBOARD_0
    if (ii >= 0x27 && ii <= 0x32) return VK_F1  + (ii - 0x27);  // F1..F12
    if (ii >= 0x5b && ii <= 0x5d) return VK_F13 + (ii - 0x5b);  // F13..F15
    if (ii >= 0x0b && ii <= 0x13) return VK_NUMPAD1 + (ii - 0x0b);  // NUMPAD1..9
    switch (ii) {
    case 0x06: return VK_RETURN;       // KEYBOARD_RETURN
    case 0x07: return VK_LEFT;
    case 0x08: return VK_RIGHT;
    case 0x09: return VK_UP;
    case 0x0a: return VK_DOWN;
    case 0x14: return VK_NUMPAD0;
    case 0x15: return VK_DECIMAL;      // NUMPADDECIMAL
    case 0x16: return VK_SUBTRACT;     // NUMPADMINUS
    case 0x17: return VK_ADD;          // NUMPADPLUS
    case 0x18: return VK_LSHIFT;
    case 0x19: return VK_RSHIFT;
    case 0x1a: return VK_LMENU;
    case 0x1b: return VK_RMENU;
    case 0x1c: return VK_LCONTROL;
    case 0x1d: return VK_RCONTROL;
    case 0x1e: return VK_TAB;
    case 0x1f: return VK_ESCAPE;
    case 0x20: return VK_HOME;
    case 0x21: return VK_END;
    case 0x22: return VK_PRIOR;        // PAGEUP
    case 0x23: return VK_NEXT;         // PAGEDOWN
    case 0x24: return VK_INSERT;
    case 0x25: return VK_DELETE;
    case 0x26: return VK_SNAPSHOT;     // PRINTSCREEN
    case 0x57: return VK_SPACE;
    case 0x58: return VK_RETURN;       // NUMPADENTER
    case 0x59: return VK_CAPITAL;
    case 0x5a: return VK_PAUSE;
    case 0x5e: return VK_OEM_MINUS;
    case 0x5f: return VK_OEM_PLUS;     // EQUALS
    case 0x60: return VK_BACK;
    case 0x61: return VK_OEM_4;        // LBRACKET
    case 0x62: return VK_OEM_6;        // RBRACKET
    case 0x63: return VK_OEM_1;        // SEMICOLON
    case 0x64: return VK_OEM_7;        // APOSTROPHE
    case 0x65: return VK_OEM_3;        // GRAVE
    case 0x66: return VK_OEM_5;        // BACKSLASH
    case 0x67: return VK_OEM_COMMA;
    case 0x68: return VK_OEM_PERIOD;
    case 0x69: return VK_OEM_2;        // SLASH
    case 0x6a: return VK_MULTIPLY;
    case 0x6c: return VK_DIVIDE;
    case 0x6d: return VK_OEM_102;      // OEM_102 (<>| key)
    default:   return 0;
    }
}

}  // namespace acc::engine_keymap
