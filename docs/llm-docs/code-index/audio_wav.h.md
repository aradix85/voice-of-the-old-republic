# audio_wav.h (46 lines)

Source: `patches/Accessibility/audio_wav.h`

In-memory RIFF/WAVE image builder.

Produces the byte layout the engine's sound code hands to Miles (AIL_set_3D_sample_file takes a complete WAV file image in memory). Used by LoopSource::StartFromMemory to play audio that has no file behind it — rendered speech, synthesised test tones. See docs/llm-docs/in-memory-sound-resource.md.

## Declarations (in source order)

- L21 `bool BuildMonoPcm16Wav(const float* samples, size_t frames, size_t channels, size_t sampleRate, std::vector<unsigned char>& out)`: Interleaved float samples (any channel count, [-1, 1]) → a mono 16-bit PCM WAV image.
- L26 `struct Levels`: Peak and RMS of a sample block in dBFS (0 = full scale; a silent block reads as -inf).
- L30 `Levels MeasureLevels(const std::vector<float>& samples)`
- L36 `float NormalizePeak(std::vector<float>& samples, float targetPeakDb)`: Scales `samples` in place so the loudest sample sits at `targetPeakDb` dBFS, giving every voice and line the same headroom before the game's own volume chain.
- L42 `void ApplyHighShelf(std::vector<float>& samples, size_t channels, size_t sampleRate, float cornerHz, float gainDb)`: RBJ-cookbook high-shelf biquad (shelf slope 1) over interleaved `samples` in place: frequencies above `cornerHz` move by `gainDb` (negative = cut), those below stay.
