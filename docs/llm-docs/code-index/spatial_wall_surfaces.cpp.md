# spatial_wall_surfaces.cpp (153 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.cpp`

The wall and floor caches of the current area — see spatial_wall_surfaces.h.

## Declarations (in source order)

- L24 `void Tick()`
- L36 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`
- L43 `void* CachedArea()`
- L47 `void Clear()`
- L52 `void RebuildForArea(void* area)`
- L82 `bool NearestWalkableFloor(const Vector& p, float maxM, Vector& outPoint)`
- L87 `bool FloorSpotNear(const Vector& p, float maxM, Vector& outSpot)`
- L91 `bool OverWalkableFloor(const Vector& p)`
- L95 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`
