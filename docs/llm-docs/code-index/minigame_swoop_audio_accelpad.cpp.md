# minigame_swoop_audio_accelpad.cpp (320 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

The accelerator-pad cues of the swoop race, per tick: the panned guide tone toward the next pad (or a detour point), the "aligned" blip on reaching it, and the crossing score. The detours and the magnet and wall cues live in minigame_swoop_audio_accelpad_detour.cpp and _assist.cpp.

The guide is our own acc_boost tone, not the bike's hum: the swoop Player's <Engine> resref is empty in every area, so player+0x144 holds no source we could pan. It aims straight at the next pad's lane, since the pads are scattered gates rather than a racing line. The pan keeps the target's true X and Z but a fixed forward depth (kSwoopGuidePanForwardRefM), so it encodes the lane offset alone; loudness rises as the target nears. The tone rests within about the catch radius (kSwoopGuideSilenceU), where the magnet finishes: the player positions coarsely by ear and never has to null the pan.

## Declarations (in source order)

- L56 `bool ReadTickInputs(void* miniGame, PadTick& t)`: The listener (the camera, else the player) and the bike's readings.
- L77 `bool IsMineModel(const char* name)`: Pads and mines are both CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads, 24 mines); the model name tells them apart (mgf_acpad / mgf_mine01), as in the track scripts.
- L91 `void ScanEnemies(void* mgoArray, PadTick& t)`: Walk the enemy pool: cache the mines for the obstacle sweep, and find the gate to aim at, the nearest pad strictly ahead whose lateral position is plausible (one junk read put a pad at X=1933).
- L156 `void UpdateLateralVelocity(const PadTick& t)`: Lateral velocity in units per tick, clamped and smoothed.
- L171 `void ScoreCrossing(const PadTick& t)`: When the targeted gate changes, the previous one has just been passed: interpolate its lateral miss at fwdGap 0 between its last sample in front and this tick, and score it against the engine's real hit test.
- L207 `void ChooseTarget(PadTick& t)`: The nearest pad, or after the last pad on KOTOR 2 a point straight ahead (see kVirtualTargetAheadU), so the mine-aware line keeps working to the finish.
- L225 `bool UpdateGuideTone(const PadTick& t)`: The panned steering tone (see the top of the file); returns whether it sounds.
- L265 `void TraceTick(const PadTick& t, bool guideActive)`
- L281 `void RememberTarget(const PadTick& t)`: This tick's gate, for next tick's crossing interpolation.
- L297 `void TickAccelpadCues(void* miniGame)`
