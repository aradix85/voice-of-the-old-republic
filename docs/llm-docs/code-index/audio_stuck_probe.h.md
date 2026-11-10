# audio_stuck_probe.h (21 lines)

Source: `patches/Accessibility/audio_stuck_probe.h`

The free-directions probe: when the leader walks without getting anywhere, or walks in circles (stuck_detect.h), speak once which of the eight compass directions are free of walls and bodies for 1.5 m.

## Declarations (in source order)

- L15 `void Tick(const Vector& pos, uint64_t now_ms, bool walking)`: Per judged position sample, from footstep_suppress::Tick.
- L18 `void Reset()`: No player: both triggers start over.
