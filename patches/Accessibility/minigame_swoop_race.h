// Swoop race minigame accessibility.
//
// Detects the race through the engine pointer CSWCArea.mini_game (type 1;
// type 2 is the turret game, minigame_turret.cpp), not through the area tag:
// the engine sets it when the minigame controller is built at module load,
// so the transition is one-shot, while tag matching would miss modded
// modules and fire on dialog-only swoop screens. On entry it speaks the
// opener and the keys; during the race the gear, the moment to shift, the
// obstacle and pad audio (minigame_swoop_audio.h), a stuck hint and the
// race time.

#pragma once

namespace acc::swoop_race {

// Once per engine tick; cheap when no race is running.
void Tick();

bool IsActive();

// Shift up a gear. KOTOR 2: run the player bike's OnAccelerate track script
// as the engine's client input handler does on its own events. KOTOR 1:
// press the player's MGshoot key (Space by default), the engine's own shift.
// False when there is no active swoop race. Tick calls it for the SwoopShift
// hotkey (W) on both games.
bool TryShift();

// KOTOR 2 only (KOTOR 1 has no jump): run the OnBrake track script, the jump.
// False when there is no active K2 swoop race. The input pipeline calls it for
// Space (event 0xd9), the engine's own jump event, so every jump is logged.
bool TryJump();

}  // namespace acc::swoop_race
