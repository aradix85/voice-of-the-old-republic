# floor_puzzle_plates.h (53 lines)

Source: `patches/Accessibility/floor_puzzle_plates.h`

Engine-free model of the floor-plate puzzle board: the plate slots, their trigger polygons and the point-versus-plate geometry floor_puzzle.cpp polls.

## Declarations (in source order)

- L17 `struct Plate`
- L30 `void ComputeCenterAndSpan(Plate& p)`: Fills center (the vertex mean) and the axis-aligned span from poly.
- L32 `bool PointInPoly(const Plate& p, float x, float y)`
- L35 `float DistToPlate(const Plate& p, float x, float y)`: 0 when inside; else the closest XY distance to any polygon edge.
- L40 `struct SpanOffset`: Signed metres from a point to the plate's span on each axis: positive when the plate lies north / east, 0 on an axis where the point is within the span.
- L44 `SpanOffset OffsetToSpan(const Plate& p, const Vector& pos)`
- L48 `int TrigSlotForTag(const char* tag)`: Trigger tag -> slot.
- L50 `int PlcSlotForTag(const char* tag)`

Also 4 constants or variables at namespace scope.
