# minigame_swoop_audio.cpp (209 lines)

Source: `patches/Accessibility/minigame_swoop_audio.cpp`

Swoop race spatial-audio implementation — see swoop_spatial_audio.h.

Split from swoop_race.cpp on 2026-05-27 (large-file-handling pass). The race lifecycle (entry/exit, gear watch, wall collision, acceleration speedometer) stays in swoop_race.cpp; everything that walks the 255-slot CSWMiniGameObjectArray to surface obstacles and accelpads as 3D loops lives here.

## Declarations (in source order)

- L95 `bool TrackAxisUsable(void* miniGame)`
- L134 `const char* ReadAurObjectName(void* aurObject)`: Returns the CAurObject's name pointer (engine-owned, valid for the lifetime of the object), or nullptr on any null link / SEH fault.
- L153 `void TickSpatialAudio(void* miniGame)`
- L161 `ParkedBar GetParkedBarSituation()`
- L165 `void ResetSpatialAudio()`
