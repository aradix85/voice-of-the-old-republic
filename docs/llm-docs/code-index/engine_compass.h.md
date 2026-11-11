# engine_compass.h (42 lines)

Source: `patches/Accessibility/engine_compass.h`

Engine yaw ↔ compass yaw ↔ 8-point sector helpers. Pure math, no state.

Frames: Engine  — 0° = +X = East, CCW positive, [0, 360). From ExecuteCommandGetFacing / GetPlayerYawDegrees. Compass — 0° = North, 90° = East, CW positive, [0, 360). What screen readers think in.

EngineYawToCompass is involutive (apply twice = identity); camera_announce relies on this for inverse projection.

Sectors: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW. Nearest-sector with half-sector offset; no hysteresis (callers wrap with their own).

## Declarations (in source order)

- L25 `float EngineYawToCompass(float engineYawDeg)`
- L26 `int CompassToSector(float compassDeg)`
- L27 `acc::strings::Id SectorString(int sector)`
- L31 `float VectorEngineYaw(float dx, float dy)`: The floor-plan vector (dx, dy), +X east and +Y north: its engine yaw and its compass bearing, both in [0, 360], and its sector.
- L32 `float VectorCompass(float dx, float dy)`
- L33 `int VectorSector(float dx, float dy)`
- L39 `int ClockPosition(float playerYawDeg, float dx, float dy)`: Clock position of the world-space offset (dx, dy) relative to a facing given in ENGINE yaw degrees: 12 = directly ahead, 3 = right, 6 = behind, 9 = left.

Also 2 constants or variables at namespace scope.
