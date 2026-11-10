# floor_puzzle_plates.cpp (90 lines)

Source: `patches/Accessibility/floor_puzzle_plates.cpp`

Floor-plate puzzle board model — see floor_puzzle_plates.h.

## Declarations (in source order)

- L15 `float ToSpan(float v, float lo, float hi)`: Signed distance from v to [lo, hi]: positive when the span lies above.
- L23 `void ComputeCenterAndSpan(Plate& p)`
- L43 `bool PointInPoly(const Plate& p, float x, float y)`
- L57 `float DistToPlate(const Plate& p, float x, float y)`
- L68 `SpanOffset OffsetToSpan(const Plate& p, const Vector& pos)`
- L72 `int TrigSlotForTag(const char* tag)`
- L81 `int PlcSlotForTag(const char* tag)`
