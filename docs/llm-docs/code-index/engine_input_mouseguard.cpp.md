# engine_input_mouseguard.cpp (415 lines)

Source: `patches/Accessibility/engine_input_mouseguard.cpp`

The no-mouse crash guard on CExoRawInputInternal::InitializeDirectInputMouse and the teardown block behind it.

## Declarations (in source order)

- L134 `const char* TeardownSiteName(int index, int total)`
- L154 `void WriteJmpRel32(uint8_t* at, uintptr_t target)`: JMP rel32.
- L171 `void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput, void* /*edx*/)`: Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the failure branches of InitializeDirectInputMouse (the engine's own shutdown path still calls the real thing).
- L210 `void InstallDirectInputMouseGuard()`
- L330 `void InstallMouseTeardownBlock()`
