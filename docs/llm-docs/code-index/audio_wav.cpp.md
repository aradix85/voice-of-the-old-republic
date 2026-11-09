# audio_wav.cpp (113 lines)

Source: `patches/Accessibility/audio_wav.cpp`

(From audio_wav.h.) In-memory RIFF/WAVE image builder.

Produces the byte layout the engine's sound code hands to Miles (AIL_set_3D_sample_file takes a complete WAV file image in memory). Used by LoopSource::StartFromMemory to play audio that has no file behind it — rendered speech, synthesised test tones. See docs/llm-docs/in-memory-sound-resource.md.

## Declarations (in source order)

- L11 `void PutU16(unsigned char* p, uint16_t v)`
- L16 `void PutU32(unsigned char* p, uint32_t v)`
- L27 `bool BuildMonoPcm16Wav(const float* samples, size_t frames, size_t channels, size_t sampleRate, std::vector<unsigned char>& out)`
- L62 `Levels MeasureLevels(const std::vector<float>& samples)`
- L75 `float NormalizePeak(std::vector<float>& samples, float targetPeakDb)`
- L84 `void ApplyHighShelf(std::vector<float>& samples, size_t channels, size_t sampleRate, float cornerHz, float gainDb)`
