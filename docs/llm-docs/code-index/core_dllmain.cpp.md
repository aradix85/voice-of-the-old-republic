# core_dllmain.cpp (356 lines)

Source: `patches/Accessibility/core_dllmain.cpp`

DLL entry + lazy-init plumbing.

DllMain runs under the loader lock — no LoadLibrary, no COM init, no file I/O here. Prism is initialised lazily on the first hook fire; that same once-only init detects the user's installed language from dialog.tlk and selects the matching strings + combat-anchor tables (and speech codepage) before the first utterance — on both games.

## Declarations (in source order)

- L32 `const char* LangName(acc::strings::Lang l)`
- L64 `bool TlkLooksCyrillic(FILE* fp, uint32_t entriesOffset)`: Sample the tlk's string blob and report whether it reads as Windows-1251 Cyrillic.
- L100 `acc::strings::Lang DetectLanguageFromTlk()`: Read the LanguageID from /dialog.tlk to determine the engine locale the player has actually installed, so combat-anchor matching + Id::* speech route to the right table.
- L170 `void EnsurePrismInitialized()`: First hook to fire runs it; subsequent calls are no-ops.
- L218 `void EnsureSessionBringup()`: Everything the patch has to stand up once per session, in the order that matters.
- L287 `extern "C" void __cdecl OnRulesInit(void* /*rulesThis*/)`: CSWRules::CSWRules construction detour (hooks.toml @ 0x00552c9a).
- L299 `BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID)`
