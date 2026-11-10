# engine_input.cpp (185 lines)

Source: `patches/Accessibility/engine_input.cpp`

(From engine_input.h.) Engine input-code translation. Pure read-side; no engine state access.

## Declarations (in source order)

- L41 `const char* InputIndexNameRaw(int code)`
- L106 `const char* InputIndexName(int code)`
- L128 `int ManagerTranslateCode(int code)`: CSWGuiManager::HandleInputEvent receives KOTOR-internal "logical action" codes for navigation / system keys and translates them to InputIndices values via two inline switch statements before dispatching to the active ...
- L140 `bool PollTurnAxis(float* out)`
- L157 `bool ReadNativeStickAxes(float* x, float* y)`
- L176 `void NoteEngineKeyEvent()`
- L180 `unsigned long long LastEngineKeyEventMs()`
