# health_bands.h (39 lines)

Source: `patches/Accessibility/health_bands.h`

Health warnings in a fight: the band a creature's health is in, and per creature the band it was last told about. Engine-free.

## Declarations (in source order)

- L13 `inline int HealthBand(int hp, int maxHp)`: 0 above half, 1 at half or below, 2 at a quarter or below.
- L20 `class HealthWarnings`
