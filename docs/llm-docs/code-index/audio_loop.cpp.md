# audio_loop.cpp (267 lines)

Source: `patches/Accessibility/audio_loop.cpp`

(From audio_loop.h.) Engine-managed looping/3D source — RAII over CExoSoundSource.

Use for sustained spatial audio where PlayCue3D's one-shot model doesn't fit (continuous tones, position-updated sources, anything needing explicit Stop).

Lifecycle: Start → optional UpdatePosition per tick → Stop. Destructor auto-Stops. Single-threaded under OnUpdate.

## Declarations (in source order)

- L69 `Vector BiasForListener(const Vector& worldPosition)`: Mirrors PlayCue3D's character-relative bias so loops + impacts land at consistent world positions.
- L84 `void TeardownEngineSource(void* src, const char* whence)`: Used by Stop and the Start error path.
- L98 `LoopSource::~LoopSource()`
- L102 `bool LoopSource::Start(const char* resref, const Vector& worldPosition, bool looping, bool spatial, int priorityGroup, int volumeByte, float maxVolDist, float minVolDist)`
- L191 `void LoopSource::UpdatePosition(const Vector& worldPosition)`
- L205 `void LoopSource::UpdateVolume(int volumeByte)`
- L220 `void LoopSource::SetPitchMultiplier(float multiplier)`
- L250 `void LoopSource::Stop()`
