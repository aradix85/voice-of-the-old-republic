# floor_puzzle_plates.h (41 lines)

Source: `patches/Accessibility/floor_puzzle_plates.h`

Engine-free model of the floor-plate puzzle board: the plate slots, their trigger polygons and the point-versus-plate geometry floor_puzzle.cpp polls.

## Declarations (in source order)

- L17 `struct Plate`
- L29 `bool PointInPoly(const Plate& p, float x, float y)`
- L32 `float DistToPlate(const Plate& p, float x, float y)`: 0 when inside; else the closest XY distance to any polygon edge.
- L36 `int TrigSlotForTag(const char* tag)`: Trigger tag -> slot.
- L38 `int PlcSlotForTag(const char* tag)`

Also 4 constants or variables at namespace scope.
