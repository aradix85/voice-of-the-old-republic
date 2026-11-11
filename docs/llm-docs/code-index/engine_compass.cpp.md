# engine_compass.cpp (67 lines)

Source: `patches/Accessibility/engine_compass.cpp`

(From engine_compass.h.) Engine yaw ↔ compass yaw ↔ 8-point sector helpers. Pure math, no state.

Frames: Engine  — 0° = +X = East, CCW positive, [0, 360). From ExecuteCommandGetFacing / GetPlayerYawDegrees. Compass — 0° = North, 90° = East, CW positive, [0, 360). What screen readers think in.

EngineYawToCompass is involutive (apply twice = identity); camera_announce relies on this for inverse projection.

Sectors: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW. Nearest-sector with half-sector offset; no hysteresis (callers wrap with their own).

## Declarations (in source order)

- L15 `float EngineYawToCompass(float engineYawDeg)`
- L21 `int CompassToSector(float compassDeg)`
- L29 `float VectorEngineYaw(float dx, float dy)`
- L34 `float VectorCompass(float dx, float dy)`
- L38 `int VectorSector(float dx, float dy)`
- L42 `int ClockPosition(float playerYawDeg, float dx, float dy)`
- L51 `acc::strings::Id SectorString(int sector)`
