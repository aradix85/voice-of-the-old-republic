# minigame_swoop_audio_accelpad_detour.cpp (233 lines)

Source: `patches/Accessibility/minigame_swoop_audio_accelpad_detour.cpp`

The detours of the guide line: around bars and mines (KOTOR 2) and, for the automatic pilot, around the buoys (KOTOR 1).

## Declarations (in source order)

- L15 `void ApplyBarDetour(void* miniGame, PadTick& t)`: Bar-aware line (see kBarFreeClearU and the kPostContactU block), from the obstacle sweep's bar cache of last tick.
- L149 `void ApplyMineDetour(PadTick& t)`: Mine-aware line (see kMineClearU), checked on the final line: the pad, or the strip the bar detour chose (a strip line once passed a mine at 4 u).
- L198 `void ApplyRockDetour(PadTick& t)`: The automatic pilot's line around the buoys (KOTOR 1), from the obstacle sweep's list of last tick.
