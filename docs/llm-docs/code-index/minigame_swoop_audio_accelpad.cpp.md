# minigame_swoop_audio_accelpad.cpp (931 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad.cpp`

Accelerator pad cues of the swoop race: steering onto the next pad, the wall warning, the assist gap and release band, per tick.

## Declarations (in source order)

- L198 `inline float WallLeadTicks()`
- L202 `inline float WallEdgeOffsetU()`
- L206 `inline float AssistGapU()`
- L214 `inline float ReleaseBand()`: Post-coast miss within the assist gap reads as "on the line" → release/hold: the magnet finishes from here, so the player need not nail dead-center by ear.
- L252 `inline float MagnetEngageU()`: ----- Lateral steering magnetism (2026-06-22, shared aim-assist port) ------- The console-style "sticky aim" from the shipped turret assist, applied to the swoop bike's ONE steering axis.
- L293 `void TickAccelpadCues(void* miniGame)`
