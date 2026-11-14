# spatial_wall_surfaces.cpp (141 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.cpp`

The wall and floor caches of the current area — see spatial_wall_surfaces.h.

## Declarations (in source order)

- L24 `void Tick()`
- L36 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`
- L43 `void Clear()`
- L48 `void RebuildForArea(void* area)`
- L78 `bool NearestWalkableFloor(const Vector& p, float maxM, Vector& outPoint)`
- L83 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`
