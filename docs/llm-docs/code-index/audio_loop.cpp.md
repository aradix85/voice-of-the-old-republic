# audio_loop.cpp (427 lines)

Source: `patches/Accessibility/audio_loop.cpp`

(From audio_loop.h.) Engine-managed looping/3D source — RAII over CExoSoundSource.

Use for sustained spatial audio where PlayCue3D's one-shot model doesn't fit (continuous tones, position-updated sources, anything needing explicit Stop).

Lifecycle: Start → optional UpdatePosition per tick → Stop. Destructor auto-Stops. Single-threaded under OnUpdate.

## Declarations (in source order)

- L85 `const char* InjectResultName(InjectResult r)`
- L98 `unsigned char* At(void* base, size_t off)`
- L105 `InjectResult InjectWave(void* src, const unsigned char* wav, size_t wavBytes)`: Fills the keyless CResWave that SetResRef just gave `src` with a copy of `wav`.
- L149 `Vector BiasForListener(const Vector& worldPosition)`: Mirrors PlayCue3D's character-relative bias so loops + impacts land at consistent world positions.
- L164 `void TeardownEngineSource(void* src, const char* whence)`: Used by Stop and the Start error path.
- L178 `LoopSource::~LoopSource()`
- L182 `bool LoopSource::Start(const char* resref, const Vector& worldPosition, bool looping, bool spatial, int priorityGroup, int volumeByte, float maxVolDist, float minVolDist)`
- L189 `bool LoopSource::StartFromMemory(const char* resref, const unsigned char* wav, size_t wavBytes, const Vector& worldPosition, int priorityGroup, int volumeByte, float maxVolDist, float minVolDist)`
- L205 `bool LoopSource::StartImpl(const char* resref, const unsigned char* wav, size_t wavBytes, const Vector& worldPosition, bool looping, bool spatial, int priorityGroup, int volumeByte, float maxVolDist, float minVolDist)`
- L308 `bool LoopSource::IsVoiceAttached() const`
- L318 `void LoopSource::LogPlayState(const char* tag) const`
- L351 `void LoopSource::UpdatePosition(const Vector& worldPosition)`
- L365 `void LoopSource::UpdateVolume(int volumeByte)`
- L380 `void LoopSource::SetPitchMultiplier(float multiplier)`
- L410 `void LoopSource::Stop()`
