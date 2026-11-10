# core_dllmain.cpp (192 lines)

Source: `patches/Accessibility/core_dllmain.cpp`

DLL entry + lazy-init plumbing.

DllMain runs under the loader lock — no LoadLibrary, no COM init, no file I/O here. Speech is initialised lazily on the first hook fire, on both games.

## Declarations (in source order)

- L33 `void EnsureSpeechInitialized()`: First hook to fire runs it; subsequent calls are no-ops.
- L62 `void EnsureSessionBringup()`: Everything the patch has to stand up once per session, in the order that matters.
- L123 `extern "C" void __cdecl OnRulesInit(void* /*rulesThis*/)`: CSWRules::CSWRules construction detour (hooks.toml @ 0x00552c9a).
- L135 `BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID)`
