# minigame_swoop_audio_internal.h (233 lines)

Source: `patches/Accessibility/minigame_swoop_audio_internal.h`

Internal to the minigame_swoop_audio module (minigame_swoop_audio*.cpp); no other file includes it. The public interface is minigame_swoop_audio.h. Here: what the files share.

## Declarations (in source order)

- L41 `struct SpatialAudioState`: Module state.
- L140 `inline int SwoopVolumeByte(float dist, float range)`: Linear volume byte for a source at 3D distance `dist` within `range`: full (kSwoopVolNearByte) at the listener, ramping to kSwoopVolFarByte at the range edge.
- L154 `const char* ReadAurObjectName(void* aurObject)`: minigame_swoop_audio.cpp
- L172 `struct ObstacleTick`: The bike, the listener and this tick's collections, shared by the steps of TickObstacleCues.
- L209 `struct ObstacleView`: One obstacle as the steps see it.
- L223 `void TickObstacleCues(void* miniGame)`: minigame_swoop_audio_obstacles.cpp
- L226 `void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius)`: minigame_swoop_audio_bars.cpp
- L227 `void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t)`
- L230 `void TickAccelpadCues(void* miniGame)`: minigame_swoop_audio_accelpad.cpp

Also 13 constants or variables at namespace scope.
