# floor_puzzle_plates.cpp (103 lines)

Source: `patches/Accessibility/floor_puzzle_plates.cpp`

Floor-plate puzzle board model — see floor_puzzle_plates.h.

## Declarations (in source order)

- L14 `float DistToSegment(float px, float py, const Vector& a, const Vector& b)`
- L28 `float ToSpan(float v, float lo, float hi)`: Signed distance from v to [lo, hi]: positive when the span lies above.
- L36 `void ComputeCenterAndSpan(Plate& p)`
- L56 `bool PointInPoly(const Plate& p, float x, float y)`
- L70 `float DistToPlate(const Plate& p, float x, float y)`
- L81 `SpanOffset OffsetToSpan(const Plate& p, const Vector& pos)`
- L85 `int TrigSlotForTag(const char* tag)`
- L94 `int PlcSlotForTag(const char* tag)`
