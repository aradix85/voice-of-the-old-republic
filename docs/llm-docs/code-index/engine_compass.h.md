# engine_compass.h (33 lines)

Source: `patches/Accessibility/engine_compass.h`

Engine yaw ↔ compass yaw ↔ 8-point sector helpers. Pure math, no state.

Frames: Engine  — 0° = +X = East, CCW positive, [0, 360). From ExecuteCommandGetFacing / GetPlayerYawDegrees. Compass — 0° = North, 90° = East, CW positive, [0, 360). What screen readers think in.

EngineYawToCompass is involutive (apply twice = identity); camera_announce relies on this for inverse projection.

Sectors: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW. Nearest-sector with half-sector offset; no hysteresis (callers wrap with their own).

## Declarations (in source order)

- L21 `float EngineYawToCompass(float engineYawDeg)`
- L22 `int CompassToSector(float compassDeg)`
- L23 `acc::strings::Id SectorString(int sector)`
- L30 `int ClockPosition(float playerYawDeg, float dx, float dy)`: Clock position of the world-space offset (dx, dy) relative to a facing given in ENGINE yaw degrees: 12 = directly ahead, 3 = right, 6 = behind, 9 = left.
