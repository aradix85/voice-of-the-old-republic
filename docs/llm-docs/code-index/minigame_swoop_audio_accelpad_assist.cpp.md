# minigame_swoop_audio_accelpad_assist.cpp (222 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_assist.cpp`

The steering magnet and the wall cues of the swoop race.

## Declarations (in source order)

- L48 `inline float WallLeadTicks()`
- L52 `inline float WallEdgeOffsetU()`
- L56 `inline float AssistGapU()`
- L88 `inline float MagnetEngageU()`: ----- Lateral steering magnetism ------- The console-style "sticky aim" from the shipped turret assist, applied to the swoop bike's ONE steering axis.
- L98 `void ApplyMagnet(PadTick& t)`: Lateral steering magnetism: nudge the bike's lane (offset.x, +0x1c4) toward the guide line, gentle far, sticky near, capped per tick, on top of the player's own steering.
- L145 `void PredictWallCue(const PadTick& t)`: The wall sound ~WallLeadTicks before the bike pins, so re-steering starts sooner.
- L170 `void UpdateWallLoop(const PadTick& t)`: KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the edge on a real overshoot, in any gear, with hysteresis at the band edge.
