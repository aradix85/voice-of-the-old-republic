# prism_compass.cpp (300 lines)

Source: `patches/Accessibility/prism_compass.cpp`

The compass channel: a second SAPI instance with its own voice, volume and rate for the camera-turn readout, and the follow-up line spoken on the normal channel once the compass voice has finished.

## Declarations (in source order)

- L35 `int ClampPercent(int v)`
- L37 `int LoadPercent(PercentSetting& s)`
- L45 `void StorePercent(PercentSetting& s, int v)`
- L52 `void ApplyCompassLevels()`: Push the compass volume and rate to its instance.
- L66 `bool EnsureCompassVoice(size_t voiceId)`: Make the compass instance speak with voice `voiceId`.
- L81 `size_t DefaultCompassVoiceId()`
- L93 `void ApplyCompassVoice()`: The saved voice if it is still installed, else the default.
- L119 `bool TryCreateCompass()`: Create the compass instance.
- L149 `void DropCompassFollowUp(const char* why)`: Forget a waiting follow-up: newer speech has made it stale.
- L158 `void SpeakCompass(const char* text)`: The compass instance's counterpart of SpeakUrgentImpl: always interrupts, mirrors to braille, never falls back to another voice on a dispatch error (SAPI may already be mid-utterance — same double-speak reasoning).
- L184 `static DWORD CompassSpeechMs(const char* text)`: How long the compass voice takes to say `text`, silence trimmed (renders come back gated); 0 when it cannot be measured.
- L193 `void SpeakCompassThen(const char* text, const char* followUp)`
- L228 `void Tick()`
- L261 `void SetCompassVolumePercent(int percent)`
- L266 `int GetCompassVolumePercent()`
- L270 `void SetCompassRatePercent(int percent)`
- L275 `int GetCompassRatePercent()`
- L279 `size_t CompassVoiceCount()`
- L283 `size_t GetCompassVoiceId()`
- L288 `void SetCompassVoiceId(size_t voiceId)`
