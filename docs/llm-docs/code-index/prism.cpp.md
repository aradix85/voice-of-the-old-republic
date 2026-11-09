# prism.cpp (364 lines)

Source: `patches/Accessibility/prism.cpp`

Speech bridge — Prism only. Loads prism.dll, picks the screen-reader backend (whichever Prism picks at startup; acquire_best) and sends it every line.

Braille: Tolk's Tolk_Output spoke and brailled in one call; Prism splits those. Speech therefore goes through DispatchNormal — prism_backend_output (the Tolk_Output equivalent) where the backend offers it, else speak plus a braille flash message.

prism.dll is loaded at runtime, so a missing one leaves the mod silent instead of failing to load.

## Declarations (in source order)

- L48 `bool Resolve(HMODULE lib, T& fn, const char* name)`
- L67 `const char* SehCodeName(DWORD code)`
- L78 `PrismBackend* SehAcquireBest(PrismContext* ctx, DWORD* outCode)`: Returns acquire_best's backend, or null.
- L89 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Acquire one specific backend by id, SEH-guarded.
- L101 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`: Initialize one backend, SEH-guarded.
- L118 `bool BackendIsUsable(PrismBackend* be)`: True iff this backend can speak AND its reader is reachable right now.
- L126 `void AdoptNormal(PrismBackend* be)`: Install `be` as the normal backend and record which braille channels it offers, so Speak can reach a braille display and not just the synthesizer.
- L138 `const char* BrailleModeText()`: How the adopted backend reaches a braille display, for the log — the line a braille user's report gets checked against.
- L147 `void BrailleNormal(const char* utf8)`: Mirrors text to the braille display, when the backend has one.
- L163 `PrismError DispatchNormal(const char* utf8, bool interrupt)`: Sends one announcement to the normal backend on every channel it offers: a backend advertising SUPPORTS_OUTPUT gets prism_backend_output, the rest get speak plus, where supported, a braille flash message.
- L181 `void SpeakNormalUtf8(const char* utf8, bool interrupt)`
- L195 `PrismBackend* WalkForLiveBackend()`: Walk the registry ourselves and take the first backend that is live and initialises.
- L230 `bool TryAcquireNormal()`: Pick the screen-reader / TTS backend.
- L263 `bool Init()`
- L326 `bool IsAvailable()`
- L333 `void Speak(const wchar_t* text, bool interrupt)`: Every Speak call is logged so the patch log carries one canonical record of what the screen reader was asked to say, in order.
- L341 `void Speak(const char* text, bool interrupt)`
- L349 `void Silence()`
- L355 `void Shutdown()`
