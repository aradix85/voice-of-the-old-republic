# prism.h (230 lines)

Source: `patches/Accessibility/prism.h`

Speech bridge — wraps the Prism C ABI.

prism.dll resolves lazily via LoadLibrary so a missing/broken install degrades to silent mode. Init MUST NOT run inside DllMain — Prism loads COM + screen-reader driver DLLs, both unsafe under the loader lock.

## Declarations (in source order)

- L15 `bool Init()`: Idempotent.
- L18 `bool IsAvailable()`: Speak/Silence are no-ops when this is false.
- L21 `void Speak(const wchar_t* text, bool interrupt)`: Normal-channel speech via the user's screen reader (NVDA/JAWS/etc).
- L25 `void Speak(const char* text, bool interrupt)`: Narrow overload — converts via MultiByteToWideChar(GetSpeechCodepage()).
- L37 `void SetSpeechCodepage(unsigned codepage)`: Codepage used to widen every narrow string on the way to the screen reader — both our own tables and engine CExoString text.
- L38 `unsigned GetSpeechCodepage()`
- L48 `void SpeakUrgent(const char* text)`: Urgent speech routes through SAPI to bypass NVDA's typed-character cancel (which silences NORMAL-priority queued speech).
- L55 `void SpeakOnUrgentChannel(const char* text, bool interrupt)`: Same urgent (SAPI) channel as SpeakUrgent, but honouring `interrupt` so a caller that emits several lines in a row gets them queued instead of each one cutting off the last.
- L65 `void SetUrgentVolumePercent(int percent)`: Urgent-channel (SAPI) volume as a user-facing percent [0,100].
- L66 `int GetUrgentVolumePercent()`
- L78 `void SetUrgentRatePercent(int percent)`: Urgent-channel (SAPI) speech rate as a user-facing percent [0,100].
- L79 `int GetUrgentRatePercent()`
- L93 `size_t UrgentVoiceCount()`: ---- Urgent-channel voice selection -------------------------------------- The urgent channel speaks with one of the SAPI voices installed on the machine.
- L98 `bool GetUrgentVoiceName(size_t voiceId, char* outBuf, size_t bufSize)`: Name of voice `voiceId`, converted to the game codepage so it can go straight to Speak / SpeakUrgent or into a composed UI line.
- L103 `size_t GetUrgentVoiceId()`: Index of the voice the urgent channel currently speaks with.
- L107 `void SetUrgentVoiceId(size_t voiceId)`: Switch the urgent channel to `voiceId` and persist it by name.
- L117 `void SpeakCompass(const char* text)`: ---- Compass channel ----------------------------------------------------- A third voice, for the facing direction — the compass readout while the camera turns and the facing key: a second SAPI instance beside the urg...
- L125 `void SpeakCompassThen(const char* text, const char* followUp)`: `text` on the compass voice, then `followUp` on the screen reader once the compass voice has said its words (measured by a silent render, so the voice's trailing silence is not waited out) — the two take turns.
- L128 `void Tick()`: Releases SpeakCompassThen's follow-up when its turn comes.
- L132 `void SetCompassVolumePercent(int percent)`: Volume and rate, percent [0,100], same contract and defaults as the urgent pair (100% volume, rate 80); persisted under their own keys.
- L133 `int GetCompassVolumePercent()`
- L134 `void SetCompassRatePercent(int percent)`
- L135 `int GetCompassRatePercent()`
- L142 `size_t CompassVoiceCount()`: The voice.
- L143 `size_t GetCompassVoiceId()`
- L144 `void SetCompassVoiceId(size_t voiceId)`
- L161 `struct RenderedSpeech`: ---- Render to memory (positional-speech groundwork) --------------------- Renders `text` (game codepage) to PCM with the urgent channel's SAPI voice instead of speaking it — prism_backend_speak_to_memory.
- L166 `bool RenderUrgentToMemory(const char* text, int ratePercent, RenderedSpeech& out)`
- L172 `inline int OffsetRatePercent(int ratePercent, int offset)`: `ratePercent` (0..100, -1 = the voice's default) moved by `offset` points, clamped to 0..100.
- L190 `bool RenderSapiVoiceToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`: Render with a NAMED voice and rate — used to imitate the screen reader when it speaks through a SAPI or OneCore voice.
- L192 `bool RenderOneCoreToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
- L199 `void NoteSpokenElsewhere(const char* text)`: A line the mod speaks itself, outside both channels (positional speech renders and plays its own audio).
- L202 `void Silence()`: Cancel in-progress speech on every channel.
- L213 `const char* LastSpoken()`: ---- Last-utterance capture -------------------------------------------- Every Speak/SpeakUrgent that actually reaches a backend records its text here, so a hotkey can act on "the thing I just heard" without each subs...
- L219 `void SuppressNextCapture()`: One-shot: the next Speak/SpeakUrgent call does not overwrite LastSpoken().
- L222 `const wchar_t* ActiveScreenReader()`: e.g.
- L227 `void Shutdown()`: Drops IsAvailable() flag; Prism context stays — tearing it down from DLL_PROCESS_DETACH is unsafe under the loader lock.
