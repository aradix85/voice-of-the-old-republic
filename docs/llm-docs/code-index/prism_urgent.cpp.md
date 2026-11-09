# prism_urgent.cpp (252 lines)

Source: `patches/Accessibility/prism_urgent.cpp`

The urgent channel: a SAPI backend that bypasses the screen reader's typed-character cancel. Its voice, volume and rate persist in the mod settings; the helpers for persisted levels and voices serve the compass channel too.

## Declarations (in source order)

- L29 `void EnsureSapiVoiceLoaded()`
- L42 `void ApplySapiVoice()`: Apply the persisted voice choice to the freshly-acquired backend.
- L58 `void ApplyUrgentLevels()`
- L64 `int LoadPercent(PercentSetting& s)`
- L72 `void StorePercent(PercentSetting& s, int v)`
- L80 `void ApplyLevels(PrismBackend* be, PercentSetting& volume, PercentSetting& rate, const char* channel)`: Push a channel's volume and rate to its backend; Prism takes both as 0.0..1.0.
- L91 `bool SetVoiceCached(PrismBackend* be, size_t& current, size_t voiceId, const char* channel)`: Make `be` speak with voice `voiceId`, remembered in `current` so a repeat is free.
- L110 `bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize)`: Copy voice `voiceId`'s name into outBuf as UTF-8 — Prism's own encoding, and what the persisted setting stores.
- L122 `size_t FindSapiVoiceByNameUtf8(const char* utf8Name)`: Index of the voice named `utf8Name`, or kNoVoice when the machine no longer has it (uninstalled voice, moved settings file, language pack removed).
- L133 `bool TryAcquireSapi()`: Acquire the SAPI backend explicitly.
- L193 `void SetUrgentVolumePercent(int percent)`
- L198 `int GetUrgentVolumePercent()`
- L202 `void SetUrgentRatePercent(int percent)`
- L207 `int GetUrgentRatePercent()`
- L211 `size_t UrgentVoiceCount()`
- L215 `bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize)`
- L227 `size_t GetUrgentVoiceId()`
- L231 `void SetUrgentVoiceId(size_t voiceId)`
