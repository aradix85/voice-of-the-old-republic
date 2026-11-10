# engine_keymap_codes.cpp (107 lines)

Source: `patches/Accessibility/engine_keymap_codes.cpp`

The engine's key codes as Windows sees them: InputIndices values to virtual keys and to DirectInput scancodes. Engine-free.

## Declarations (in source order)

- L19 `int InputIndexToScancode(int ii)`: Engine InputIndices value -> DIK (PS/2 set-1) scancode, DIRECTLY.
- L44 `int InputIndexToVk(int ii)`
