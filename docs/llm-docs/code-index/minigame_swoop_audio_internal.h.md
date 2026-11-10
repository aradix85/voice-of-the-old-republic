# minigame_swoop_audio_internal.h (631 lines)

Source: `patches/Accessibility/minigame_swoop_audio_internal.h`

Internal to the minigame_swoop_audio module (minigame_swoop_audio*.cpp); no other file includes it. The public interface is minigame_swoop_audio.h. Here: what the files share.

## Declarations (in source order)

- L186 `enum HazardClass`
- L188 `enum HazardState`
- L189 `struct HazardInfo`
- L436 `struct SpatialAudioState`: Module state.
- L526 `inline int SwoopVolumeByte(float dist, float range)`: Linear volume byte for a source at 3D distance `dist` within `range`: full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte at the range edge.
- L540 `const char* ReadAurObjectName(void* aurObject)`: minigame_swoop_audio.cpp
- L575 `struct ObstacleTick`: The bike, the listener and this tick's collections, shared by the steps of TickObstacleCues.
- L607 `struct ObstacleView`: One obstacle as the steps see it.
- L621 `void TickObstacleCues(void* miniGame)`: minigame_swoop_audio_obstacles.cpp
- L624 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: minigame_swoop_audio_bars.cpp
- L625 `void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t)`
- L628 `void TickAccelpadCues(void* miniGame)`: minigame_swoop_audio_accelpad.cpp

Also 82 constants or variables at namespace scope.
