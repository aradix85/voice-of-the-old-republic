# prism_render.cpp (159 lines)

Source: `patches/Accessibility/prism_render.cpp`

Speech rendered to memory (PCM) instead of spoken, on render-only SAPI and OneCore instances: for positional speech and for timing a line.

## Declarations (in source order)

- L11 `PrismError SehSpeakToMemory(PrismBackend* be, const char* utf8, PFN_PrismAudioCallback cb, void* userdata, DWORD* outCode)`: Render one utterance to memory, SEH-guarded.
- L24 `void __cdecl CollectRenderedSpeech(void* userdata, const float* samples, size_t sampleCount, size_t channels, size_t sampleRate)`: Copies the rendered block out of Prism's buffer, which is only valid for the duration of the callback.
- L36 `static bool RenderWith(PrismBackend* be, const char* tag, const char* text, RenderedSpeech& out)`: Shared body of the RenderXToMemory entry points.
- L62 `static PrismBackend* EnsureRenderBackend(RenderBackend& r)`
- L82 `static bool VoiceNameMatches(const char* name, const char* wanted)`: Does Prism's voice name `name` denote the voice described as `wanted`?
- L90 `static bool SelectVoice(RenderBackend& r, const char* wanted)`: Point `r` at the first voice matching `wanted` (UTF-8).
- L113 `float RateFromPercent(int percent)`: Prism rate (0..1, 0.5 = the voice's default) from a percent.
- L120 `bool RenderOn(RenderBackend& r, const char* voice, float rate, float volume, const char* text, RenderedSpeech& out)`: One render on a render-only backend.
- L131 `bool RenderUrgentToMemory(const char* text, int ratePercent, RenderedSpeech& out)`
- L144 `bool RenderSapiVoiceToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
- L151 `bool RenderOneCoreToMemory(const char* voiceName, int ratePercent, const char* text, RenderedSpeech& out)`
