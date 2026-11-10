# minigame_swoop_audio_accelpad.cpp (354 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

Accelerator pad cues of the swoop race: steering onto the next pad, the wall warning, the assist gap and release band, per tick.

## Declarations (in source order)

- L91 `bool ReadTickInputs(void* miniGame, PadTick& t)`: The listener (the camera, else the player) and the bike's readings.
- L112 `bool IsMineModel(const char* name)`: Pads and mines are both CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads, 24 mines); the model name tells them apart (mgf_acpad / mgf_mine01), as in the track scripts.
- L126 `void ScanEnemies(void* mgoArray, PadTick& t)`: Walk the enemy pool: cache the mines for the obstacle sweep, and find the gate to aim at, the nearest pad strictly ahead whose lateral position is plausible (one junk read put a pad at X=1933).
- L191 `void UpdateLateralVelocity(const PadTick& t)`: Lateral velocity in units per tick, clamped and smoothed.
- L206 `void ScoreCrossing(const PadTick& t)`: When the targeted gate changes, the previous one has just been passed: interpolate its lateral miss at fwdGap 0 between its last sample in front and this tick, and score it against the engine's real hit test.
- L242 `void ChooseTarget(PadTick& t)`: The nearest pad, or after the last pad on KOTOR 2 a point straight ahead (see kVirtualTargetAheadU), so the mine-aware line keeps working to the finish.
- L259 `bool UpdateGuideTone(const PadTick& t)`: The panned steering tone (see the block above); returns whether it sounds.
- L299 `void TraceTick(const PadTick& t, bool guideActive)`
- L315 `void RememberTarget(const PadTick& t)`: This tick's gate, for next tick's crossing interpolation.
- L331 `void TickAccelpadCues(void* miniGame)`
