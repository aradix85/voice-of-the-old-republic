# audio_loop.h (88 lines)

Source: `patches/Accessibility/audio_loop.h`

Engine-managed looping/3D source — RAII over CExoSoundSource.

Use for sustained spatial audio where PlayCue3D's one-shot model doesn't fit (continuous tones, position-updated sources, anything needing explicit Stop).

Lifecycle: Start → optional UpdatePosition per tick → Stop. Destructor auto-Stops. Single-threaded under OnUpdate.

## Declarations (in source order)

- L18 `class LoopSource`
