# audio_bus.h (226 lines)

Source: `patches/Accessibility/audio_bus.h`

Audio cue playback — thin wrappers over CExoSound's one-shot entry points. Goes through the engine's own audio pipeline so the SFX slider, focus-pause, EAX reverb, and voice budgeting all apply automatically.

CExoSound* lives at *kAddrCExoSoundPtr; can be nullptr early in DLL-attach. All functions are nothrow, SEH-guarded, and return false on any failure. True = cue queued (doesn't guarantee audibility — engine may evict under voice-budget pressure, resref may not resolve).

## Declarations (in source order)

- L35 `void* GetSoundInternal()`: The engine's CExoSoundInternal (CExoSound facade +0x00), or nullptr while the sound system is not up yet.
- L43 `void SetGlobalCueVolumePercent(int percent)`: --- Global cue volume (the user-facing "hint sounds" slider) -------- Percent in [0,100]; 100 = each cue at its base volume, 0 = muted.
- L44 `int GetGlobalCueVolumePercent()`
- L52 `uint8_t GetCuePriorityGroup()`: Priority group our one-shot cues ride.
- L60 `uint8_t GetSpatialCuePriorityGroup()`: Near-field cue group — the installer's second sentinel row, a tight 1m/8m falloff band so loudness tracks distance across the passive proximity cues' ~5m awareness range (the flat group above is full-volume out to 10m...
- L74 `bool PlayCue(const char* resref, uint8_t priorityGroup = 0, uint8_t baseVolume = kCueVolumeFull)`: 2D one-shot cue (centred, no spatial position).
- L94 `bool PlayCue3D(const char* resref, const Vector& worldPosition, uint8_t priorityGroup = 0, uint8_t baseVolume = kCueVolumeFull)`: 3D positional one-shot.
- L109 `bool PlayCue3DRaw(const char* resref, const Vector& listenerSpacePosition, uint8_t priorityGroup = 0, uint8_t baseVolume = kCueVolumeFull)`: Same one-shot WITHOUT the (camera - character) shift: the position is taken as already relative to the engine's own listener (the camera).
- L120 `struct CResRef`: Engine resource-reference tag: a fixed 16-byte, NOT necessarily NUL-terminated char buffer.
- L124 `void FillResRef(CResRef& out, const char* tag)`

Also 21 constants or variables at namespace scope.
