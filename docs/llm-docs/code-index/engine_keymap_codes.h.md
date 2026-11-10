# engine_keymap_codes.h (25 lines)

Source: `patches/Accessibility/engine_keymap_codes.h`

Key-code translation of engine_keymap (engine_keymap_codes.cpp). InputIndexToVk is declared in engine_keymap.h.

## Declarations (in source order)

- L10 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK scancode, by physical key.
- L15 `struct KeymapEntry`: One [Keymapping] line of swkotor.ini: "Action [A|B]= ".
- L22 `bool ParseKeymapEntry(const char* line, KeymapEntry& out)`: False for a line without '='.
