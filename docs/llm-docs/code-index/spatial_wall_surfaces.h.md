# spatial_wall_surfaces.h (45 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.h`

The wall and floor caches of the current area, rebuilt when the player enters one. Room shapes, the Q/E wall test, route smoothing and the stuck probe read the walls; the wall probe reads the floor heights.

## Declarations (in source order)

- L19 `void Tick()`: Main thread, once per frame: rebuilds the caches when the player is in a new area.
- L24 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`: The cached perimeter edges, portal seams filtered.
- L27 `void RebuildForArea(void* area)`: Rebuilds both caches for this area; a null area clears them.
- L30 `void Clear()`: Back to "no area": the accessors return false or empty.
- L42 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`: Floor heights of the walkmesh triangles containing (x,y), one entry per stacked storey (bridge over tunnel), exact on ramps via plane interpolation.

Also 2 constants or variables at namespace scope.
