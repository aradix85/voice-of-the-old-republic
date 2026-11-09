# prism.cpp (1684 lines)

Source: `patches/Accessibility/prism.cpp`

Speech bridge — Prism only.

Two dispatch paths:

Speak / Silence       → "normal" backend (NVDA / JAWS / ZDSR / OneCore / ... whichever screen reader Prism picks at startup; acquire_best). Same priority NVDA normally uses, so typed-character cancel etc. behaves the way the user expects from their screen reader. SpeakUrgent           → SAPI backend (always). Bypasses NVDA's typed- character cancel — the bypass we rely on for map-cursor / compass / walking cues. SpeakOnUrgentChannel  → the same SAPI backend, but the caller keeps control of interrupt-vs-queue. Used by the subtitle router (dialog_speech.cpp) when the user has moved dialogue onto this channel. SpeakCompass          → a second SAPI instance with its own voice, volume and rate, for the camera-turn compass readout. Same NVDA bypass as SpeakUrgent; falls back to it when the instance is missing. SpeakCompassThen      → a compass line, then a normal-channel line once the compass voice has finished (Tick releases it). The facing key: direction, then area.

Braille: Tolk's Tolk_Output spoke and brailled in one call; Prism splits those. The normal channel therefore dispatches through DispatchNormal — prism_backend_output (the Tolk_Output equivalent) where the backend offers it, else speak plus a braille flash message — and SpeakUrgent mirrors its SAPI alerts to the normal backend's braille channel, since SAPI has no braille display of its own.

All Prism functions are resolved at runtime via LoadLibrary + GetProcAddress so a missing prism.dll degrades silently (no speech).

## Declarations (in source order)

- L178 `void EnsureSapiVoiceLoaded()`
- L198 `void EnsureSapiVolumeLoaded()`
- L215 `void EnsureSapiRateLoaded()`
- L258 `void ApplySapiVolume()`: Push g_sapiVolumePercent to the live SAPI backend.
- L277 `void ApplySapiRate()`: Push g_sapiRatePercent to the live SAPI backend.
- L300 `bool EnsureSapiVoice(size_t voiceId)`: Make the SAPI backend speak with voice `voiceId`.
- L319 `bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize)`: Copy voice `voiceId`'s name into outBuf as UTF-8 — Prism's own encoding, and what the persisted setting stores.
- L332 `size_t FindSapiVoiceByNameUtf8(const char* utf8Name)`: Index of the voice named `utf8Name`, or SIZE_MAX when the machine no longer has it (uninstalled voice, moved save file, language pack removed).
- L347 `void ApplySapiVoice()`: Apply the persisted voice choice to the freshly-acquired backend.
- L364 `bool Resolve(HMODULE lib, T& fn, const char* name, bool required = true)`
- L389 `bool ReencodeAcpToUtf8(const char* in, char* outBuf, size_t outBufSize)`: Game codepage → UTF-8.
- L422 `bool ReencodeUtf8ToGameCodepage(const char* in, char* outBuf, size_t outBufSize)`: Mirror of the above: Prism hands voice names back in UTF-8, and everything downstream of us (speech dispatch, the composed UI line) works in the game codepage.
- L460 `void CaptureSpoken(const char* text)`
- L474 `void CaptureSpokenW(const wchar_t* text)`: Wide path: fold back to the game codepage so LastSpoken() has one encoding regardless of which overload produced it.
- L510 `const char* SehCodeName(DWORD code)`: ---- SEH guards around prism backend calls ----------------------------- Some screen-reader backends delay-load a vendor DLL inside their initialize() path (ZDSR → ZDSRAPI.dll, PC-Talker → PCTKUSR.dll, BoyPC → BoyCtrl...
- L521 `PrismBackend* SehAcquireBest(PrismContext* ctx, DWORD* outCode)`: Returns acquire_best's backend, or null.
- L532 `PrismBackend* SehAcquire(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Acquire one specific backend by id, SEH-guarded.
- L543 `PrismBackend* SehCreate(PrismContext* ctx, PrismBackendId id, DWORD* outCode)`: Create a fresh backend instance by id, SEH-guarded.
- L555 `PrismError SehInitialize(PrismBackend* be, DWORD* outCode)`: Initialize one backend, SEH-guarded.
- L566 `PrismError SehSpeakToMemory(PrismBackend* be, const char* utf8, PFN_PrismAudioCallback cb, void* userdata, DWORD* outCode)`: Render one utterance to memory, SEH-guarded.
- L579 `void __cdecl CollectRenderedSpeech(void* userdata, const float* samples, size_t sampleCount, size_t channels, size_t sampleRate)`: Copies the rendered block out of Prism's buffer, which is only valid for the duration of the callback.
- L589 `bool TryAcquireSapi()`: Acquire the SAPI backend explicitly and bump its rate.
- L693 `int ClampPercent(int v)`
- L695 `int LoadPercent(PercentSetting& s)`
- L703 `void StorePercent(PercentSetting& s, int v)`
- L710 `void ApplyCompassLevels()`: Push the compass volume and rate to its instance.
- L724 `bool EnsureCompassVoice(size_t voiceId)`: Make the compass instance speak with voice `voiceId`.
- L739 `size_t DefaultCompassVoiceId()`
- L751 `void ApplyCompassVoice()`: The saved voice if it is still installed, else the default.
- L775 `bool TryCreateCompass()`: Create the compass instance.
- L820 `void DropCompassFollowUp(const char* why)`: Forget a waiting follow-up: newer speech has made it stale.
- L839 `bool BackendIsUsable(PrismBackend* be)`: True iff this backend can speak AND its reader is reachable right now.
- L849 `void AdoptNormal(PrismBackend* be)`: Install `be` as the normal backend and record which braille channels it offers, so Speak can reach a braille display and not just the synthesizer.
- L862 `const char* BrailleModeText()`: How the adopted backend reaches a braille display, for the log — the line a braille user's report gets checked against.
- L874 `void BrailleNormal(const char* utf8)`: Mirrors text to the braille display through the normal backend, when it has one.
- L894 `PrismError DispatchNormal(const char* utf8, bool interrupt)`: Sends one announcement to the normal backend on every channel it offers.
- L921 `PrismBackend* WalkForLiveBackend()`: Walk the registry ourselves and take the first backend that is live and initialises.
- L964 `bool TryAcquireNormal()`: Pick the screen-reader / TTS backend for the normal channel.
- L997 `bool Init()`
- L1114 `bool IsAvailable()`
- L1122 `void Speak(const wchar_t* text, bool interrupt)`: Self-log every Speak call so the patch log carries one canonical record of what the screen reader was asked to say, in chronological order.
- L1157 `void SetSpeechCodepage(unsigned codepage)`
- L1164 `unsigned GetSpeechCodepage()`
- L1166 `void Speak(const char* text, bool interrupt)`
- L1203 `static bool AllViaScreenReader()`: SpeakUrgent core.
- L1208 `static void SpeakUrgentImpl(const char* text, bool interrupt)`
- L1282 `void SpeakUrgent(const char* text)`
- L1286 `void NoteSpokenElsewhere(const char* text)`
- L1295 `void SpeakOnUrgentChannel(const char* text, bool interrupt)`
- L1302 `void SpeakCompass(const char* text)`: The compass instance's counterpart of SpeakUrgentImpl: always interrupts, mirrors to braille, never falls back to another voice on a dispatch error (SAPI may already be mid-utterance — same double-speak reasoning).
- L1327 `static bool RenderWith(PrismBackend* be, const char* tag, const char* text, RenderedSpeech& out)`: Shared body of the RenderXToMemory entry points.
- L1369 `static PrismBackend* EnsureRenderBackend(RenderBackend& r)`
- L1389 `static bool VoiceNameMatches(const char* name, const char* wanted)`: Does Prism's voice name `name` denote the voice described as `wanted`?
- L1397 `static bool SelectVoice(RenderBackend& r, const char* wanted)`: Point `r` at the first voice matching `wanted` (UTF-8).
- L1420 `static float RateFromPercent(int percent)`: Prism rate (0..1, 0.5 = the voice's default) from a percent.
- L1427 `static bool RenderOn(RenderBackend& r, const char* voice, float rate, float volume, const char* text, RenderedSpeech& out)`: One render on a render-only backend.
- L1438 `bool RenderUrgentToMemory(const char* text, int ratePercent, RenderedSpeech& out)`
- L1451 `bool RenderSapiVoiceToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
- L1458 `bool RenderOneCoreToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
- L1468 `static DWORD CompassSpeechMs(const char* text)`: How long the compass voice takes to say `text`, silence trimmed (renders come back gated); 0 when it cannot be measured.
- L1477 `void SpeakCompassThen(const char* text, const char* followUp)`
- L1512 `void Tick()`
- L1545 `void SetUrgentVolumePercent(int percent)`
- L1554 `int GetUrgentVolumePercent()`
- L1559 `void SetUrgentRatePercent(int percent)`
- L1568 `int GetUrgentRatePercent()`
- L1573 `size_t UrgentVoiceCount()`
- L1577 `bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize)`
- L1591 `size_t GetUrgentVoiceId()`
- L1597 `void SetUrgentVoiceId(size_t voiceId)`
- L1617 `void SetCompassVolumePercent(int percent)`
- L1622 `int GetCompassVolumePercent()`
- L1626 `void SetCompassRatePercent(int percent)`
- L1631 `int GetCompassRatePercent()`
- L1635 `size_t CompassVoiceCount()`
- L1639 `size_t GetCompassVoiceId()`
- L1644 `void SetCompassVoiceId(size_t voiceId)`
- L1655 `void Silence()`
- L1664 `const wchar_t* ActiveScreenReader()`
- L1669 `const char* LastSpoken()`
- L1671 `void SuppressNextCapture()`
- L1673 `void Shutdown()`
