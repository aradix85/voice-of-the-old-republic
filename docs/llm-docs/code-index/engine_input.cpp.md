# engine_input.cpp (175 lines)

Source: `patches/Accessibility/engine_input.cpp`

The CExoInput facade: the InputIndices names for the logs, the manager's code translation, the camera turn axis and stick, and the time of the last engine key event. The mouse guard and the DirectInput acquire live in engine_input_mouseguard.cpp and engine_input_acquire.cpp.

## Declarations (in source order)

- L50 `void* ExoInput()`
- L56 `const char* InputIndexName(int code)`: The InputIndices enum (Lane's SARIF, /KotOR Types/Enums/InputIndices): index = value; -1 is INPUTDEVICE_NONE.
- L128 `int ManagerTranslateCode(int code)`: CSWGuiManager::HandleInputEvent (0x0040c8e0) translates the engine's logical navigation codes to InputIndices before the panel sees them: 0xb4 (180), 0xdf (223) -> 0x28 = KEYBOARD_F2 (cancel / back) 0xb5 (181), 0xbb (...
- L140 `bool PollTurnAxis(float* out)`
- L152 `bool ReadNativeStickAxes(float* x, float* y)`
- L166 `void NoteEngineKeyEvent()`
- L170 `unsigned long long LastEngineKeyEventMs()`
