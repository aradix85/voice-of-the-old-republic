# engine_input_mouseguard.cpp (407 lines)

Source: `patches/Accessibility/engine_input_mouseguard.cpp`

The no-mouse crash guard on CExoRawInputInternal::InitializeDirectInputMouse and the teardown block behind it.

## Declarations (in source order)

- L130 `const char* TeardownSiteName(int index, int total)`
- L147 `void WriteJmpRel32(uint8_t* at, uintptr_t target)`: JMP rel32 from `at` to `target`.
- L164 `void __fastcall MouseInitFailedInsteadOfTeardown(void* rawInput, void* /*edx*/)`: Stand-in for CExoRawInputInternal::ShutDownDirectInput, reached only from the failure branches of InitializeDirectInputMouse (the engine's own shutdown path still calls the real thing).
- L203 `void InstallDirectInputMouseGuard()`
- L322 `void InstallMouseTeardownBlock()`
