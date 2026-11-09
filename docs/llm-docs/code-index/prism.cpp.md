# prism.cpp (650 lines)

Source: `patches/Accessibility/prism.cpp`

Speech bridge — Prism only.

Two dispatch paths:

Speak / Silence       → "normal" backend (NVDA / JAWS / ZDSR / OneCore / ... whichever screen reader Prism picks at startup; acquire_best). Same priority NVDA normally uses, so typed-character cancel etc. behaves the way the user expects from their screen reader. SpeakUrgent           → SAPI backend (always). Bypasses NVDA's typed- character cancel — the bypass we rely on for map-cursor / compass / walking cues. SpeakOnUrgentChannel  → the same SAPI backend, but the caller keeps control of interrupt-vs-queue. Used by the subtitle router (dialog_speech.cpp) when the user has moved dialogue onto this channel. SpeakCompass          → a second SAPI instance with its own voice, volume and rate, for the camera-turn compass readout. Same NVDA bypass as SpeakUrgent; falls back to it when the instance is missing. SpeakCompassThen      → a compass line, then a normal-channel line once the compass voice has finished (Tick releases it). The facing key: direction, then area.

Braille: Tolk's Tolk_Output spoke and brailled in one call; Prism splits those. The normal channel therefore dispatches through DispatchNormal — prism_backend_output (the Tolk_Output equivalent) where the backend offers it, else speak plus a braille flash message — and SpeakUrgent mirrors its SAPI alerts to the normal backend's braille channel, since SAPI has no braille display of its own.

All Prism functions are resolved at runtime via LoadLibrary + GetProcAddress so a missing prism.dll degrades silently (no speech).

## Declarations (in source order)

- L74 `bool Resolve(HMODULE lib, T& fn, const char* name, bool required = true)`
- L85 `PrismBackend* SehAcquireBest(PrismContext* ctx, DWORD* outCode)`: Returns acquire_best's backend, or null.
- L108 `bool BackendIsUsable(PrismBackend* be)`: True iff this backend can speak AND its reader is reachable right now.
- L118 `void AdoptNormal(PrismBackend* be)`: Install `be` as the normal backend and record which braille channels it offers, so Speak can reach a braille display and not just the synthesizer.
- L131 `const char* BrailleModeText()`: How the adopted backend reaches a braille display, for the log — the line a braille user's report gets checked against.
- L145 `PrismError DispatchNormal(const char* utf8, bool interrupt)`: Sends one announcement to the normal backend on every channel it offers.
- L172 `PrismBackend* WalkForLiveBackend()`: Walk the registry ourselves and take the first backend that is live and initialises.
- L215 `bool TryAcquireNormal()`: Pick the screen-reader / TTS backend for the normal channel.
- L271 `const char* SehCodeName(DWORD code)`: ---- SEH guards around prism backend calls ----------------------------- Some screen-reader backends delay-load a vendor DLL inside their initialize() path (ZDSR → ZDSRAPI.dll, PC-Talker → PCTKUSR.dll, BoyPC → BoyCtrl...
- L281 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Acquire one specific backend by id, SEH-guarded.
- L292 `PrismBackend* SehCreate(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Create a fresh backend instance by id, SEH-guarded.
- L304 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`: Initialize one backend, SEH-guarded.
- L318 `void BrailleNormal(const char* utf8)`: Mirrors text to the braille display through the normal backend, when it has one.
- L332 `bool Init()`
- L449 `bool IsAvailable()`
- L457 `void Speak(const wchar_t* text, bool interrupt)`: Self-log every Speak call so the patch log carries one canonical record of what the screen reader was asked to say, in chronological order.
- L492 `void Speak(const char* text, bool interrupt)`
- L529 `bool AllViaScreenReader()`: SpeakUrgent core.
- L534 `void SpeakUrgentImpl(const char* text, bool interrupt)`
- L608 `void SpeakUrgent(const char* text)`
- L612 `void NoteSpokenElsewhere(const char* text)`
- L621 `void SpeakOnUrgentChannel(const char* text, bool interrupt)`
- L625 `void Silence()`
- L634 `const wchar_t* ActiveScreenReader()`
- L639 `void Shutdown()`
