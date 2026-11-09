# prism_urgent.cpp (309 lines)

Source: `patches/Accessibility/prism_urgent.cpp`

The urgent channel: a SAPI backend that bypasses the screen reader's typed-character cancel. Its voice, volume and rate persist in the mod settings.

## Declarations (in source order)

- L21 `void EnsureSapiVoiceLoaded()`
- L30 `void EnsureSapiVolumeLoaded()`
- L44 `void ApplySapiVolume()`: Push g_sapiVolumePercent to the live SAPI backend.
- L63 `void ApplySapiRate()`: Push g_sapiRatePercent to the live SAPI backend.
- L86 `bool EnsureSapiVoice(size_t voiceId)`: Make the SAPI backend speak with voice `voiceId`.
- L106 `void ApplySapiVoice()`: Apply the persisted voice choice to the freshly-acquired backend.
- L124 `void EnsureSapiRateLoaded()`
- L137 `bool FetchSapiVoiceNameUtf8(size_t voiceId, char* outBuf, size_t bufSize)`: Copy voice `voiceId`'s name into outBuf as UTF-8 — Prism's own encoding, and what the persisted setting stores.
- L150 `size_t FindSapiVoiceByNameUtf8(const char* utf8Name)`: Index of the voice named `utf8Name`, or SIZE_MAX when the machine no longer has it (uninstalled voice, moved save file, language pack removed).
- L162 `bool TryAcquireSapi()`: Acquire the SAPI backend explicitly and bump its rate.
- L236 `void SetUrgentVolumePercent(int percent)`
- L245 `int GetUrgentVolumePercent()`
- L250 `void SetUrgentRatePercent(int percent)`
- L259 `int GetUrgentRatePercent()`
- L264 `size_t UrgentVoiceCount()`
- L268 `bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize)`
- L282 `size_t GetUrgentVoiceId()`
- L288 `void SetUrgentVoiceId(size_t voiceId)`
