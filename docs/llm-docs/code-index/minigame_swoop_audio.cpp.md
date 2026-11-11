# minigame_swoop_audio.cpp (183 lines)

Source: `patches/Accessibility/minigame_swoop_audio.cpp`

Swoop race spatial audio, core: track-axis guard and the public entry points of minigame_swoop_audio.h.

The race lifecycle (entry/exit, gear watch, wall collision, acceleration speedometer) lives in minigame_swoop_race.cpp; everything that walks the 255-slot CSWMiniGameObjectArray to surface obstacles and accelpads as 3D loops lives here.

## Declarations (in source order)

- L74 `bool TrackAxisUsable(void* miniGame)`
- L113 `const char* ReadAurObjectName(void* aurObject)`: Returns the CAurObject's name pointer (engine-owned, valid for the lifetime of the object), or nullptr on any null link / SEH fault.
- L128 `void TickSpatialAudio(void* miniGame)`
- L136 `ParkedBar GetParkedBarSituation()`
- L140 `void ResetSpatialAudio()`
