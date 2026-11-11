# minigame_swoop_audio_accelpad_internal.h (63 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_internal.h`

Internal to the accelerator-pad cues (minigame_swoop_audio_accelpad*.cpp); no other file includes it. Here: what those files share.

## Declarations (in source order)

- L16 `struct PadTick`: One tick of the pad cues: the readings, the nearest pad ahead, and the line the guide tone and the magnet steer to.
- L53 `void ApplyBarDetour(void* miniGame, PadTick& t)`: minigame_swoop_audio_accelpad_detour.cpp
- L54 `void ApplyMineDetour(PadTick& t)`
- L55 `void ApplyRockDetour(PadTick& t)`
- L58 `void ApplyMagnet(PadTick& t)`: minigame_swoop_audio_accelpad_assist.cpp
- L59 `void PredictWallCue(const PadTick& t)`
- L60 `void UpdateWallLoop(const PadTick& t)`

Also 1 constants or variables at namespace scope.
