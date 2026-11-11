# minigame_swoop_audio_accelpad_assist.cpp (209 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_assist.cpp`

The steering magnet and the wall cues of the swoop race.

## Declarations (in source order)

- L42 `float MagnetEngageU()`
- L46 `float WallLeadTicks()`
- L50 `float WallEdgeOffsetU()`
- L61 `Pull MagnetPull(const PadTick& t, float s, const acc::minigame::MagnetParams& mp)`: The pull at closeness `s` (0 at the engage edge, 1 on the line).
- L95 `void ApplyMagnet(PadTick& t)`: Lateral steering magnetism.
- L132 `void PredictWallCue(const PadTick& t)`: The wall sound ~WallLeadTicks before the bike pins, so re-steering starts sooner.
- L157 `void UpdateWallLoop(const PadTick& t)`: KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the edge on a real overshoot, in any gear, with hysteresis at the band edge.
