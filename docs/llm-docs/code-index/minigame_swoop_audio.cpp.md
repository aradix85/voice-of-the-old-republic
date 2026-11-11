# minigame_swoop_audio.cpp (197 lines)

Source: `patches/Accessibility/minigame_swoop_audio.cpp`

Swoop race spatial audio, core: track-axis guard and the public entry points of minigame_swoop_audio.h.

The race lifecycle (entry/exit, gear watch, wall collision, acceleration speedometer) lives in minigame_swoop_race.cpp; everything that walks the 255-slot CSWMiniGameObjectArray to surface obstacles and accelpads as 3D loops lives here.

## Declarations (in source order)

- L76 `bool TrackAxisUsable(void* miniGame)`
- L115 `const char* ReadAurObjectName(void* aurObject)`: Returns the CAurObject's name pointer (engine-owned, valid for the lifetime of the object), or nullptr on any null link / SEH fault.
- L130 `void TickSpatialAudio(void* miniGame)`
- L138 `ParkedBar GetParkedBarSituation()`
- L142 `bool AutopilotOn()`
- L148 `void ResetSpatialAudio()`
