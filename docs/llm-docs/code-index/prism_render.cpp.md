# prism_render.cpp (156 lines)

Source: `patches/Accessibility/prism_render.cpp`

Speech rendered to memory (PCM) instead of spoken, on render-only SAPI and OneCore instances: for positional speech and for timing a line.

## Declarations (in source order)

- L31 `PrismError SehSpeakToMemory(PrismBackend* be, const char* utf8, PFN_PrismAudioCallback cb, void* userdata, DWORD* outCode)`: Render one utterance to memory, SEH-guarded.
- L44 `void __cdecl CollectRenderedSpeech(void* userdata, const float* samples, size_t sampleCount, size_t channels, size_t sampleRate)`: Copies the rendered block out of Prism's buffer, which is only valid for the duration of the callback.
- L52 `PrismBackend* EnsureRenderBackend(RenderBackend& r)`
- L71 `bool VoiceNameMatches(const char* name, const char* wanted)`: Does Prism's voice name `name` denote the voice described as `wanted`?
- L79 `bool SelectVoice(RenderBackend& r, const char* wanted)`: Point `r` at the first voice matching `wanted` (UTF-8).
- L99 `bool RenderOn(RenderBackend& r, const char* voice, int ratePercent, const char* text, RenderedSpeech& out)`: One render at full volume on a render-only backend.
- L132 `bool RenderUrgentToMemory(const char* text, int ratePercent, RenderedSpeech& out)`
- L143 `bool RenderSapiVoiceToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
- L149 `bool RenderOneCoreToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
