# spatial_wall_surfaces.h (114 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.h`

Wall cache + surface clustering, rebuilt when the player enters an area. Room shapes, the Q/E wall test, route smoothing and the view-mode cursor all read it.

The walkmesh stores wall geometry as many short edges (Endar Spire corridors are ~0.7-1m per edge). Adjacent collinear edges are clustered into "surfaces" at area-load time, so consumers can reason about one physical wall instead of its fragments.

Cluster rule: two edges merge if they share an endpoint (within kEndpointTolMeters) AND their direction vectors agree within kSurfaceCollinearityCosThreshold. Cross-room merging is intentional — KOTOR's "room" is an internal .lyt segmentation and a physical wall regularly spans multiple rooms. The geometric tests decide alone.

Each clustered surface is also reduced to a single straight segment (two extreme endpoints + unit direction + length) so room_topology doesn't have to re-merge the edge soup.

## Declarations (in source order)

- L42 `struct WallSurfaceDesc`: One wall surface = chain of collinear endpoint-sharing edges clustered at area-load via union-find.
- L54 `void Tick()`: Main thread, once per frame: rebuilds the caches when the player is in a new area.
- L59 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`: The cached perimeter edges, portal seams filtered.
- L65 `void RebuildForArea(void* area)`: Rebuild for this area.
- L69 `void Clear()`: Reset to "no area" state.
- L74 `const acc::engine::WallEdge* GetWallBuffer()`: Borrowed pointer into static storage, valid until the next RebuildForArea or Clear.
- L80 `const int* GetEdgeSurfaceIdBuffer()`: Parallel array to GetWallBuffer(): element i is the surface index for wall edge i.
- L83 `int GetWallCount()`: 0 before first RebuildForArea.
- L86 `int GetSurfaceCount()`: 0 before first RebuildForArea.
- L91 `bool GetSurfaceDesc(int idx, WallSurfaceDesc& outDesc)`: False on out-of-range, before clustering runs, or for surfaces that couldn't reduce to a single straight segment (closed-loop / branching edge cases flagged with edge_count == 0).
- L96 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`: 2D segment-vs-surface intersection.
- L111 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`: Floor heights of the walkmesh triangles containing (x,y), one entry per stacked storey (bridge over tunnel), exact on ramps via plane interpolation.

Also 3 constants or variables at namespace scope.
