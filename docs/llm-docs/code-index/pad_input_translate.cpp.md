# pad_input_translate.cpp (365 lines)

Source: `patches/Accessibility/pad_input_translate.cpp`

KOTOR 2's two gamepad seams: normalise pad events at the GUI manager (TranslateManagerEvent) and claim chords on the in-world client route (TranslateClientEvent), plus telling a pad event from its keyboard twin.

## Declarations (in source order)

- L33 `bool PhysDown(int vk)`: Pad-vs-keyboard discrimination
- L44 `int KeyboardTwinVk(int code)`: The keyboard row each shared code belongs to.
- L65 `bool IsPadCode(int code)`
- L81 `const char* CodeHint(int code)`
- L101 `bool IsPhysicalF1()`
- L106 `bool TranslateClientEvent(int code, int value)`
- L143 `Verdict TranslateManagerEvent(int& code, int& value)`
