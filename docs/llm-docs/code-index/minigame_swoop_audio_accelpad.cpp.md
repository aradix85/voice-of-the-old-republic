# minigame_swoop_audio_accelpad.cpp (836 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

Accelerator pad cues of the swoop race: steering onto the next pad, the wall warning, the assist gap and release band, per tick.

## Declarations (in source order)

- L152 `inline float WallLeadTicks()`
- L156 `inline float WallEdgeOffsetU()`
- L160 `inline float AssistGapU()`
- L192 `inline float MagnetEngageU()`: ----- Lateral steering magnetism ------- The console-style "sticky aim" from the shipped turret assist, applied to the swoop bike's ONE steering axis.
- L266 `bool ReadTickInputs(void* miniGame, PadTick& t)`: The listener (the camera, else the player) and the bike's readings.
- L287 `bool IsMineModel(const char* name)`: Pads and mines are both CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads, 24 mines); the model name tells them apart (mgf_acpad / mgf_mine01), as in the track scripts.
- L301 `void ScanEnemies(void* mgoArray, PadTick& t)`: Walk the enemy pool: cache the mines for the obstacle sweep, and find the gate to aim at, the nearest pad strictly ahead whose lateral position is plausible (one junk read put a pad at X=1933).
- L366 `void UpdateLateralVelocity(const PadTick& t)`: Lateral velocity in units per tick, clamped and smoothed.
- L381 `void ScoreCrossing(const PadTick& t)`: When the targeted gate changes, the previous one has just been passed: interpolate its lateral miss at fwdGap 0 between its last sample in front and this tick, and score it against the engine's real hit test.
- L417 `void ChooseTarget(PadTick& t)`: The nearest pad, or after the last pad on KOTOR 2 a point straight ahead (see kVirtualTargetAheadU), so the mine-aware line keeps working to the finish.
- L434 `void ApplyBarDetour(void* miniGame, PadTick& t)`: Bar-aware line (see kBarFreeClearU and the kPostContactU block), from the obstacle sweep's bar cache of last tick.
- L563 `void ApplyMineDetour(PadTick& t)`: Mine-aware line (see kMineClearU), checked on the final line: the pad, or the strip the bar detour chose (a strip line once passed a mine at 4 u).
- L615 `void ApplyMagnet(PadTick& t)`: Lateral steering magnetism: nudge the bike's lane (offset.x, +0x1c4) toward the guide line, gentle far, sticky near, capped per tick, on top of the player's own steering.
- L662 `void PredictWallCue(const PadTick& t)`: The wall sound ~WallLeadTicks before the bike pins, so re-steering starts sooner.
- L687 `void UpdateWallLoop(const PadTick& t)`: KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the edge on a real overshoot, in any gear, with hysteresis at the band edge.
- L741 `bool UpdateGuideTone(const PadTick& t)`: The panned steering tone (see the block above); returns whether it sounds.
- L781 `void TraceTick(const PadTick& t, bool guideActive)`
- L797 `void RememberTarget(const PadTick& t)`: This tick's gate, for next tick's crossing interpolation.
- L813 `void TickAccelpadCues(void* miniGame)`
