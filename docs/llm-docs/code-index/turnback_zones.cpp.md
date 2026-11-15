# turnback_zones.cpp (67 lines)

Source: `patches/Accessibility/turnback_zones.cpp`

Turn-back zones (turnback_zones.h).

## Declarations (in source order)

- L18 `bool Holds(const Zone& z, const Vector& p)`
- L24 `void ForArea(const void* area)`
- L30 `bool Add(const Vector* poly, int count)`
- L45 `void AppendWalls(const Vector& a, const Vector& b, std::vector<acc::engine::WallEdge>& out)`
- L57 `bool Crosses(const Vector& a, const Vector& b)`
