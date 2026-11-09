# audio_pitch.h (29 lines)

Source: `patches/Accessibility/audio_pitch.h`

Pitch-variance neutraliser for accessibility cues.

BeginScopedZero / EndScopedZero bracket the engine's PlayOneShotSound calls in audio_bus.cpp. While the per-thread flag is set, the detour at CExoSoundSourceInternal::CalculatePitchVarianceFrequency returns the source's base frequency instead of the engine's randomised value — pitch jitter shifts HRTF response, which degrades localisation and breaks per-cue identification.

thread_local rather than global: only the source we just asked the engine to create gets neutralised. Resref-matching would also catch engine-driven plays of the same resrefs (Inventory_Add, etc.).

## Declarations (in source order)

- L20 `void BeginScopedZero()`: Pair 1:1, Begin before the engine call, End after even on failure.
- L21 `void EndScopedZero()`
- L24 `bool IsScopedZeroActive()`: Read from the detour handler.
- L28 `extern "" int __cdecl OnCalculatePitchVarianceFrequency(void* source)`
