# spatial_wall_surfaces.h (61 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.h`

The wall and floor caches of the current area, rebuilt when the player enters one. Room shapes, the Q/E wall test, route smoothing and the stuck probe read the walls; the wall probe reads the floor heights.

## Declarations (in source order)

- L19 `void Tick()`: Main thread, once per frame: rebuilds the caches when the player is in a new area.
- L24 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`: The cached perimeter edges, portal seams filtered.
- L28 `void* CachedArea()`: The area Tick last built the caches for; null without a player.
- L31 `void RebuildForArea(void* area)`: Rebuilds both caches for this area; a null area clears them.
- L34 `void Clear()`: Back to "no area": the accessors return false or empty.
- L46 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`: Floor heights of the walkmesh triangles containing (x,y), one entry per stacked storey (bridge over tunnel), exact on ramps via plane interpolation.
- L50 `bool NearestWalkableFloor(const Vector& p, float maxM, Vector& outPoint)`: acc::engine::NearestWalkableFloor over the cached floor: where the floor a creature can stand on comes nearest to p.
- L54 `bool FloorSpotNear(const Vector& p, float maxM, Vector& outSpot)`: acc::engine::FloorSpotNear over the cached floor: where to stand to be as near to p as the floor allows.
- L58 `bool OverWalkableFloor(const Vector& p)`: acc::engine::OverWalkableFloor over the cached floor.

Also 2 constants or variables at namespace scope.
