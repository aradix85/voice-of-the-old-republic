# audio_bus.cpp (439 lines)

Source: `patches/Accessibility/audio_bus.cpp`

(From audio_bus.h.) Audio cue playback — thin wrappers over CExoSound's one-shot entry points. Goes through the engine's own audio pipeline so the SFX slider, focus-pause, EAX reverb, and voice budgeting all apply automatically.

CExoSound* lives at *kAddrCExoSoundPtr; can be nullptr early in DLL-attach. All functions are nothrow, SEH-guarded, and return false on any failure. True = cue queued (doesn't guarantee audibility — engine may evict under voice-budget pressure, resref may not resolve).

## Declarations (in source order)

- L20 `void FillResRef(CResRef& out, const char* tag)`: Declared in audio_bus.h so audio_loop.cpp shares it rather than mirroring.
- L60 `void* GetCExoSound()`
- L77 `void EnsureCueVolumeLoaded()`
- L89 `uint8_t EffectiveVolumeByte(uint8_t baseVolume)`: baseVolume × slider, rounded, clamped to [0,127].
- L137 `void* GetSoundInternal()`
- L148 `void SetGlobalCueVolumePercent(int percent)`
- L157 `int GetGlobalCueVolumePercent()`
- L171 `int ScanForSentinelGroup(uint16_t sentinel)`
- L208 `uint8_t GetCuePriorityGroup()`
- L233 `bool PlayCue(const char* resref, uint8_t priorityGroup, uint8_t baseVolume)`
- L268 `bool PlayCue3DImpl(const char* resref, const Vector& worldPosition, uint8_t priorityGroup, uint8_t baseVolume, bool characterRelative)`: Shared body of PlayCue3D / PlayCue3DRaw.
- L324 `bool PlayCue3D(const char* resref, const Vector& worldPosition, uint8_t priorityGroup, uint8_t baseVolume)`
- L330 `bool PlayCue3DRaw(const char* resref, const Vector& listenerSpacePosition, uint8_t priorityGroup, uint8_t baseVolume)`
- L369 `extern "C" int __cdecl OnSetListenerPosition(void* exoSound, Vector** posSlot)`
