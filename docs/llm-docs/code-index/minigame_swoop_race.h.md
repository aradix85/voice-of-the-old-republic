# minigame_swoop_race.h (34 lines)

Source: `patches/Accessibility/minigame_swoop_race.h`

Swoop race minigame accessibility.

Detects the race through the engine pointer CSWCArea.mini_game (type 1; type 2 is the turret game, minigame_turret.cpp), not through the area tag: the engine sets it when the minigame controller is built at module load, so the transition is one-shot, while tag matching would miss modded modules and fire on dialog-only swoop screens. On entry it speaks the opener and the keys; during the race the gear, the moment to shift, the obstacle and pad audio (minigame_swoop_audio.h), a stuck hint and the race time.

## Declarations (in source order)

- L17 `void Tick()`: Once per engine tick; cheap when no race is running.
- L19 `bool IsActive()`
- L26 `bool TryShift()`: Shift up a gear.
- L31 `bool TryJump()`: KOTOR 2 only (KOTOR 1 has no jump): run the OnBrake track script, the jump.
