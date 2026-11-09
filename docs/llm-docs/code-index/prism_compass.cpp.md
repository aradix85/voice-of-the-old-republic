# prism_compass.cpp (259 lines)

Source: `patches/Accessibility/prism_compass.cpp`

The compass channel: a second SAPI instance with its own voice, volume and rate for the camera-turn readout, and the follow-up line spoken on the normal channel once the compass voice has finished.

A voice of its own lets the player tell a heading from an announcement by the voice alone, and neither cuts the other off — each instance owns its own SpVoice and audio stream. prism_registry_create builds it fresh; prism_registry_acquire would hand back the urgent channel's cached one.

Volume, rate and voice persist like the urgent channel's, the voice by name. Without a saved voice the channel takes kCompassDefaultVoiceId, or the first voice that is not the urgent one when that id is missing or is the urgent voice itself: the default only has to sound different from the announcements. It is re-derived each launch until the player chooses one.

## Declarations (in source order)

- L47 `void ApplyCompassLevels()`
- L51 `bool SetCompassVoice(size_t voiceId)`
- L55 `size_t DefaultCompassVoiceId()`
- L67 `void ApplyCompassVoice()`: The saved voice if it is still installed, else the default.
- L92 `DWORD CompassSpeechMs(const char* text)`: How long the compass voice takes to say `text`, silence trimmed (renders come back gated); 0 when it cannot be measured.
- L104 `bool TryCreateCompass()`: Create the compass instance.
- L130 `void DropCompassFollowUp(const char* why)`: Forget a waiting follow-up: newer speech has made it stale.
- L139 `void SpeakCompass(const char* text)`: The compass instance's counterpart of SpeakUrgentImpl: always interrupts, mirrors to braille, never falls back to another voice on a dispatch error (SAPI may already be mid-utterance — same double-speak reasoning).
- L162 `void SpeakCompassThen(const char* text, const char* followUp)`
- L188 `void Tick()`
- L221 `void SetCompassVolumePercent(int percent)`
- L226 `int GetCompassVolumePercent()`
- L230 `void SetCompassRatePercent(int percent)`
- L235 `int GetCompassRatePercent()`
- L239 `size_t CompassVoiceCount()`
- L243 `size_t GetCompassVoiceId()`
- L247 `void SetCompassVoiceId(size_t voiceId)`
