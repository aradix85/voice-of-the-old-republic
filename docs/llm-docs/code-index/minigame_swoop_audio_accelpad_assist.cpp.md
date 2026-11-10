# minigame_swoop_audio_accelpad_assist.cpp (187 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_assist.cpp`

The steering magnet and the wall cues of the swoop race.

## Declarations (in source order)

- L41 `float MagnetEngageU()`
- L45 `float WallLeadTicks()`
- L49 `float WallEdgeOffsetU()`
- L63 `void ApplyMagnet(PadTick& t)`: Lateral steering magnetism.
- L110 `void PredictWallCue(const PadTick& t)`: The wall sound ~WallLeadTicks before the bike pins, so re-steering starts sooner.
- L135 `void UpdateWallLoop(const PadTick& t)`: KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the edge on a real overshoot, in any gear, with hysteresis at the band edge.
