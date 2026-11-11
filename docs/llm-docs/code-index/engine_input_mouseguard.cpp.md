# engine_input_mouseguard.cpp (400 lines)

Source: `patches/Accessibility/engine_input_mouseguard.cpp`

The no-mouse crash guard on CExoRawInputInternal::InitializeDirectInputMouse and the teardown block behind it.

## Declarations (in source order)

- L131 `const char* TeardownSiteName(int index, int total)`
- L157 `void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput, void* /*edx*/)`: Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the failure branches of InitializeDirectInputMouse (the engine's own shutdown path still calls the real thing).
- L196 `void InstallDirectInputMouseGuard()`
- L315 `void InstallMouseTeardownBlock()`
