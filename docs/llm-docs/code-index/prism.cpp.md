# prism.cpp (515 lines)

Source: `patches/Accessibility/prism.cpp`

Speech bridge — Prism only. This file loads prism.dll, picks the normal backend and holds the normal and urgent entry points; the other channels and helpers are in the prism_*.cpp files.

Speak / Silence       → "normal" backend (NVDA / JAWS / ZDSR / OneCore / ... whichever screen reader Prism picks at startup; acquire_best). Same priority NVDA normally uses, so typed-character cancel etc. behaves the way the user expects from their screen reader. SpeakUrgent           → SAPI backend (always). Bypasses NVDA's typed- character cancel — the bypass we rely on for map-cursor / compass / walking cues. SpeakOnUrgentChannel  → the same SAPI backend, but the caller keeps control of interrupt-vs-queue. Used by the subtitle router (dialog_speech.cpp) when the user has moved dialogue onto this channel. SpeakCompass          → a second SAPI instance with its own voice, volume and rate, for the camera-turn compass readout (prism_compass.cpp).

Braille: Tolk's Tolk_Output spoke and brailled in one call; Prism splits those. The normal channel therefore dispatches through DispatchNormal — prism_backend_output (the Tolk_Output equivalent) where the backend offers it, else speak plus a braille flash message — and SpeakUrgent mirrors its SAPI alerts to the normal backend's braille channel, since SAPI has no braille display of its own.

prism.dll is loaded at runtime, so a missing one leaves the mod silent instead of failing to load.

## Declarations (in source order)

- L72 `bool Resolve(HMODULE lib, T& fn, const char* name)`
- L93 `PrismBackend* SehAcquireBest(PrismContext* ctx, DWORD* outCode)`: Returns acquire_best's backend, or null.
- L104 `PrismBackend* SehCreate(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Create a fresh backend instance by id, SEH-guarded.
- L121 `bool BackendIsUsable(PrismBackend* be)`: True iff this backend can speak AND its reader is reachable right now.
- L129 `void AdoptNormal(PrismBackend* be)`: Install `be` as the normal backend and record which braille channels it offers, so Speak can reach a braille display and not just the synthesizer.
- L141 `const char* BrailleModeText()`: How the adopted backend reaches a braille display, for the log — the line a braille user's report gets checked against.
- L149 `PrismError DispatchNormal(const char* utf8, bool interrupt)`: Sends one announcement to the normal backend on every channel it offers: a backend advertising SUPPORTS_OUTPUT gets prism_backend_output, the rest get speak plus, where supported, a braille flash message.
- L167 `void SpeakNormalUtf8(const char* utf8, bool interrupt)`
- L181 `PrismBackend* WalkForLiveBackend()`: Walk the registry ourselves and take the first backend that is live and initialises.
- L216 `bool TryAcquireNormal()`: Pick the screen-reader / TTS backend for the normal channel.
- L249 `const char* SehCodeName(DWORD code)`
- L259 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Acquire one specific backend by id, SEH-guarded.
- L271 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`: Initialize one backend, SEH-guarded.
- L283 `PrismBackend* CreateBackend(PrismBackendId id, PrismError* rc, DWORD* seh)`: A fresh, initialised instance of backend `id`; null on failure, with the PrismError and any SEH code left for the caller's log.
- L295 `void BrailleNormal(const char* utf8)`: Mirrors text to the braille display through the normal backend, when it has one.
- L308 `bool Init()`
- L393 `bool IsAvailable()`
- L400 `void Speak(const wchar_t* text, bool interrupt)`: Every Speak call is logged so the patch log carries one canonical record of what the screen reader was asked to say, in order.
- L409 `void Speak(const char* text, bool interrupt)`
- L421 `bool AllViaScreenReader()`: Mod settings -> "Speak everything with the screen reader": the urgent (SAPI) and compass voices step aside and every line goes to the normal screen-reader channel instead.
- L430 `void SpeakUrgentImpl(const char* text, bool interrupt)`: SpeakUrgent core.
- L473 `void SpeakUrgent(const char* text)`
- L477 `void NoteSpokenElsewhere(const char* text)`
- L486 `void SpeakOnUrgentChannel(const char* text, bool interrupt)`
- L490 `void Silence()`
- L499 `const wchar_t* ActiveScreenReader()`
- L504 `void Shutdown()`
