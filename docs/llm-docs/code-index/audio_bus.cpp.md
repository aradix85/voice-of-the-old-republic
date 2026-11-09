# audio_bus.cpp (474 lines)

Source: `patches/Accessibility/audio_bus.cpp`

(From audio_bus.h.) Audio cue playback — thin wrappers over CExoSound's one-shot entry points. Goes through the engine's own audio pipeline so the SFX slider, focus-pause, EAX reverb, and voice budgeting all apply automatically.

CExoSound* lives at *kAddrCExoSoundPtr; can be nullptr early in DLL-attach. All functions are nothrow, SEH-guarded, and return false on any failure. True = cue queued (doesn't guarantee audibility — engine may evict under voice-budget pressure, resref may not resolve).

## Declarations (in source order)

- L20 `void FillResRef(CResRef& out, const char* tag)`: Declared in audio_bus.h so audio_loop.cpp shares it rather than mirroring.
- L60 `void* GetCExoSound()`
- L77 `void EnsureCueVolumeLoaded()`
- L89 `uint8_t EffectiveVolumeByte(uint8_t baseVolume)`: baseVolume × slider, rounded, clamped to [0,127].
- L143 `void* GetSoundInternal()`
- L154 `void SetGlobalCueVolumePercent(int percent)`
- L163 `int GetGlobalCueVolumePercent()`
- L177 `int ScanForSentinelGroup(uint16_t sentinel)`
- L214 `uint8_t GetCuePriorityGroup()`
- L239 `uint8_t GetSpatialCuePriorityGroup()`
- L268 `bool PlayCue(const char* resref, uint8_t priorityGroup, uint8_t baseVolume)`
- L303 `bool PlayCue3DImpl(const char* resref, const Vector& worldPosition, uint8_t priorityGroup, uint8_t baseVolume, bool characterRelative)`: Shared body of PlayCue3D / PlayCue3DRaw.
- L359 `bool PlayCue3D(const char* resref, const Vector& worldPosition, uint8_t priorityGroup, uint8_t baseVolume)`
- L365 `bool PlayCue3DRaw(const char* resref, const Vector& listenerSpacePosition, uint8_t priorityGroup, uint8_t baseVolume)`
- L404 `extern "C" int __cdecl OnSetListenerPosition(void* exoSound, Vector** posSlot)`
