# minigame_swoop_audio_internal.h (690 lines)

Source: `patches/Accessibility/minigame_swoop_audio_internal.h`

Internal to the minigame_swoop_audio module (minigame_swoop_audio*.cpp); no other file includes it. The public interface is minigame_swoop_audio.h. Here: what the files share.

## Declarations (in source order)

- L238 `enum HazardClass`
- L240 `enum HazardState`
- L241 `struct HazardInfo`
- L555 `struct SpatialAudioState`: Module state.
- L670 `inline int SwoopVolumeByte(float dist, float range)`: Linear volume byte for a source at 3D distance `dist` within `range`: full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte at the range edge.
- L680 `const char* ReadAurObjectName(void* aurObject)`
- L683 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: minigame_swoop_audio_obstacles.cpp
- L684 `void TickObstacleCues(void* miniGame)`
- L687 `void TickAccelpadCues(void* miniGame)`: minigame_swoop_audio_accelpad.cpp

Also 83 constants or variables at namespace scope.
