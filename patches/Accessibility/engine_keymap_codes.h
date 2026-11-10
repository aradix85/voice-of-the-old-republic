// Key-code translation of engine_keymap (engine_keymap_codes.cpp).
// InputIndexToVk is declared in engine_keymap.h.

#pragma once

namespace acc::engine_keymap {

// Engine InputIndices value -> DIK scancode, by physical key. 0 for keys a
// bind realistically never uses, and for Caps Lock on purpose.
int InputIndexToScancode(int ii);

// One [Keymapping] line of swkotor.ini: "Action<id>[A|B]=<InputIndices
// value, decimal>". The movement and turn actions come as slot pairs, A the
// negative direction and B the positive; slot is 0 for a slotless action.
struct KeymapEntry {
    int  actionId;    // 0 when the key is not of the Action form
    char slot;        // 'A', 'B' or 0
    int  inputIndex;
};

// False for a line without '='. `line` starts at its first non-blank.
bool ParseKeymapEntry(const char* line, KeymapEntry& out);

}  // namespace acc::engine_keymap
