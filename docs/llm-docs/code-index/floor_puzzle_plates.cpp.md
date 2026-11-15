# floor_puzzle_plates.cpp (74 lines)

Source: `patches/Accessibility/floor_puzzle_plates.cpp`

Floor-plate puzzle board model — see floor_puzzle_plates.h.

## Declarations (in source order)

- L16 `float ToSpan(float v, float lo, float hi)`: Signed distance from v to [lo, hi]: positive when the span lies above.
- L24 `void ComputeCenterAndSpan(Plate& p)`
- L44 `bool PointInPoly(const Plate& p, float x, float y)`
- L48 `float DistToPlate(const Plate& p, float x, float y)`
- L52 `SpanOffset OffsetToSpan(const Plate& p, const Vector& pos)`
- L56 `int TrigSlotForTag(const char* tag)`
- L65 `int PlcSlotForTag(const char* tag)`
