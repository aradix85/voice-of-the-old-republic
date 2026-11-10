# minigame_swoop_audio_accelpad_internal.h (113 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_internal.h`

Internal to the accelerator-pad cues (minigame_swoop_audio_accelpad*.cpp); no other file includes it. Here: what those files share.

## Declarations (in source order)

- L69 `struct PadTick`: Accelerator-pad STEERING GUIDE — continuous panned tone.
- L104 `void ApplyBarDetour(void* miniGame, PadTick& t)`: minigame_swoop_audio_accelpad_detour.cpp
- L105 `void ApplyMineDetour(PadTick& t)`
- L108 `void ApplyMagnet(PadTick& t)`: minigame_swoop_audio_accelpad_assist.cpp
- L109 `void PredictWallCue(const PadTick& t)`
- L110 `void UpdateWallLoop(const PadTick& t)`

Also 6 constants or variables at namespace scope.
