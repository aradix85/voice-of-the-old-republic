# minigame_swoop_audio_accelpad_assist.cpp (193 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_assist.cpp`

The steering magnet and the wall cues of the swoop race.

## Declarations (in source order)

- L42 `float MagnetEngageU()`
- L46 `float WallLeadTicks()`
- L50 `float WallEdgeOffsetU()`
- L67 `void ApplyMagnet(PadTick& t)`: Lateral steering magnetism.
- L116 `void PredictWallCue(const PadTick& t)`: The wall sound ~WallLeadTicks before the bike pins, so re-steering starts sooner.
- L141 `void UpdateWallLoop(const PadTick& t)`: KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the edge on a real overshoot, in any gear, with hysteresis at the band edge.
