# engine_input.cpp (844 lines)

Source: `patches/Accessibility/engine_input.cpp`

device fields moved by 4 bytes in K2

## Declarations (in source order)

- L214 `const char* TeardownSiteName(int index, int total)`
- L234 `void WriteJmpRel32(uint8_t* at, uintptr_t target)`: JMP rel32.
- L247 `const char* InputIndexNameRaw(int code)`
- L311 `const char* InputIndexName(int code)`
- L333 `int ManagerTranslateCode(int code)`: CSWGuiManager::HandleInputEvent receives KOTOR-internal "logical action" codes for navigation / system keys and translates them to InputIndices values via two inline switch statements before dispatching to the active ...
- L345 `void InstallDirectInputMouseGuard()`
- L477 `void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput, void* /*edx*/)`: Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the failure branches of InitializeDirectInputMouse (the engine's own shutdown path still calls the real thing).
- L516 `void InstallMouseTeardownBlock()`
- L600 `bool MouseDeviceUnavailable()`
- L617 `bool ReadDiSnapshot(DiSnapshot* out)`
- L647 `void LogDirectInputStateIfChanged()`
- L684 `bool EnsureInputAcquired()`
- L706 `bool ForceReacquireInput()`
- L732 `bool ReleaseInput()`
- L763 `bool PollTurnAxis(float* out)`
- L780 `bool ReadNativeStickAxes(float* x, float* y)`
- L813 `void NoteEngineKeyEvent()`
- L817 `unsigned long long LastEngineKeyEventMs()`
- L821 `void RequestInputReacquire()`
- L825 `void RequestInputRelease()`
- L829 `void DrainPendingReacquire()`
