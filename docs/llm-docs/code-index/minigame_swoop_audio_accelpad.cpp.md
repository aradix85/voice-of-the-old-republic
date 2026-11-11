# minigame_swoop_audio_accelpad.cpp (326 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

The accelerator-pad cues of the swoop race, per tick: the panned guide tone toward the next pad (or a detour point), the "aligned" blip on reaching it, and the crossing score. The detours and the magnet and wall cues live in minigame_swoop_audio_accelpad_detour.cpp and _assist.cpp; the automatic pilot's line around the buoys in minigame_swoop_pilot.cpp.

The guide is our own acc_boost tone, not the bike's hum: the swoop Player's <Engine> resref is empty in every area, so player+0x144 holds no source we could pan. It aims straight at the next pad's lane, since the pads are scattered gates rather than a racing line. The pan keeps the target's true X and Z but a fixed forward depth (kSwoopGuidePanForwardRefM), so it encodes the lane offset alone; loudness rises as the target nears. The tone rests within about the catch radius (kSwoopGuideSilenceU), where the magnet finishes: the player positions coarsely by ear and never has to null the pan.

## Declarations (in source order)

- L57 `bool ReadTickInputs(void* miniGame, PadTick& t)`: The listener (the camera, else the player) and the bike's readings.
- L78 `bool IsMineModel(const char* name)`: Pads and mines are both CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads, 24 mines); the model name tells them apart (mgf_acpad / mgf_mine01), as in the track scripts.
- L92 `void ScanEnemies(void* mgoArray, PadTick& t)`: Walk the enemy pool: cache the mines for the obstacle sweep, and find the gate to aim at, the nearest pad strictly ahead whose lateral position is plausible (one junk read put a pad at X=1933).
- L158 `void UpdateVelocity(PadTick& t)`: Lateral velocity in units per tick, clamped and smoothed, and this tick's forward travel.
- L175 `void ScoreCrossing(const PadTick& t)`: When the targeted gate changes, the previous one has just been passed: interpolate its lateral miss at fwdGap 0 between its last sample in front and this tick, and score it against the engine's real hit test.
- L212 `void ChooseTarget(PadTick& t)`: The nearest pad, or after the last pad a point straight ahead (see kVirtualTargetAheadU), so the line around the mines (KOTOR 2) and the automatic pilot's line around the buoys keep working to the finish.
- L230 `bool UpdateGuideTone(const PadTick& t)`: The panned steering tone (see the top of the file); returns whether it sounds.
- L270 `void TraceTick(const PadTick& t, bool guideActive)`
- L286 `void RememberTarget(const PadTick& t)`: This tick's gate, for next tick's crossing interpolation.
- L302 `void TickAccelpadCues(void* miniGame)`
