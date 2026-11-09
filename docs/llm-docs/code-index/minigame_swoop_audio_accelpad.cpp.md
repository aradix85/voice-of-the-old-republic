# minigame_swoop_audio_accelpad.cpp (817 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

Accelerator pad cues of the swoop race: steering onto the next pad, the wall warning, the assist gap and release band, per tick.

## Declarations (in source order)

- L152 `inline float WallLeadTicks()`
- L156 `inline float WallEdgeOffsetU()`
- L160 `inline float AssistGapU()`
- L192 `inline float MagnetEngageU()`: ----- Lateral steering magnetism ------- The console-style "sticky aim" from the shipped turret assist, applied to the swoop bike's ONE steering axis.
- L225 `void TickAccelpadCues(void* miniGame)`
