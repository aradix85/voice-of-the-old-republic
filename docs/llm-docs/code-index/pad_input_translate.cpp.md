# pad_input_translate.cpp (408 lines)

Source: `patches/Accessibility/pad_input_translate.cpp`

KOTOR 2's two gamepad seams: normalise pad events at the GUI manager (TranslateManagerEvent) and claim chords on the in-world client route (TranslateClientEvent), plus telling a pad event from its keyboard twin.

## Declarations (in source order)

- L35 `bool PhysDown(int vk)`: Pad-vs-keyboard discrimination
- L46 `int KeyboardTwinVk(int code)`: The keyboard row each shared code belongs to.
- L67 `bool IsPadCode(int code)`
- L83 `const char* CodeHint(int code)`
- L103 `bool IsPhysicalF1()`
- L108 `bool TranslateClientEvent(int code, int value)`
- L145 `Verdict TranslateManagerEvent(int& code, int& value)`
