# engine_input.cpp (862 lines)

Source: `patches/Accessibility/engine_input.cpp`

device fields moved by 4 bytes in K2

## Declarations (in source order)

- L215 `const char* TeardownSiteName(int index, int total)`
- L235 `void WriteJmpRel32(uint8_t* at, uintptr_t target)`: JMP rel32.
- L248 `const char* InputIndexNameRaw(int code)`
- L321 `const char* InputIndexName(int code)`: Public wrapper.
- L351 `int ManagerTranslateCode(int code)`: CSWGuiManager::HandleInputEvent receives KOTOR-internal "logical action" codes for navigation / system keys and translates them to InputIndices values via two inline switch statements before dispatching to the active ...
- L363 `void InstallDirectInputMouseGuard()`
- L495 `void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput, void* /*edx*/)`: Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the failure branches of InitializeDirectInputMouse (the engine's own shutdown path still calls the real thing).
- L534 `void InstallMouseTeardownBlock()`
- L618 `bool MouseDeviceUnavailable()`
- L635 `bool ReadDiSnapshot(DiSnapshot* out)`
- L665 `void LogDirectInputStateIfChanged()`
- L702 `bool EnsureInputAcquired()`
- L724 `bool ForceReacquireInput()`
- L750 `bool ReleaseInput()`
- L781 `bool PollTurnAxis(float* out)`
- L798 `bool ReadNativeStickAxes(float* x, float* y)`
- L831 `void NoteEngineKeyEvent()`
- L835 `unsigned long long LastEngineKeyEventMs()`
- L839 `void RequestInputReacquire()`
- L843 `void RequestInputRelease()`
- L847 `void DrainPendingReacquire()`
