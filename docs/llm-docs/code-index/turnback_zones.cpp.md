# turnback_zones.cpp (101 lines)

Source: `patches/Accessibility/turnback_zones.cpp`

Zones the mod's own path stays out of (turnback_zones.h).

## Declarations (in source order)

- L19 `bool Holds(const Zone& z, const Vector& p)`
- L23 `bool AddZone(const Vector* poly, int count, bool standing)`
- L39 `void Outlines(const Vector& a, const Vector& b, bool standingOnly, std::vector<acc::engine::WallEdge>& out)`
- L52 `bool CrossesOutlines(const Vector& a, const Vector& b, bool standingOnly)`
- L63 `void ForArea(const void* area)`
- L69 `bool Add(const Vector* poly, int count)`
- L73 `void ClearStanding()`
- L83 `bool AddStanding(const Vector* poly, int count)`
- L87 `void AppendWalls(const Vector& a, const Vector& b, std::vector<acc::engine::WallEdge>& out)`
- L92 `bool Crosses(const Vector& a, const Vector& b)`
- L96 `bool CrossesStanding(const Vector& a, const Vector& b)`
