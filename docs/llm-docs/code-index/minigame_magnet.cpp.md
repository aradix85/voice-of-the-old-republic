# minigame_magnet.cpp (20 lines)

Source: `patches/Accessibility/minigame_magnet.cpp`

(From minigame_magnet.h.) Sticky magnetism for the minigame aim assists: a proximity-ramped pull on one axis, gentle far from the target (a guide the player can steer straight through to pick another target) and strong on it, capped per tick so a far target is pulled and not yanked. Engine-free. What stays with each game: how the error is computed (angular for the turret, lane units for the swoop bike), which target, and the sign from offset to world.

## Declarations (in source order)

- L5 `float MagnetGain(float t, const MagnetParams& p)`
- L11 `float MagnetStep(float offsetVal, float mappedErr, float gain, const MagnetParams& p)`
