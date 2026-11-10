# spatial_wall_surfaces.cpp (571 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.cpp`

(From spatial_wall_surfaces.h.) Wall cache + surface clustering, rebuilt when the player enters an area. Room shapes, the Q/E wall test, route smoothing and the view-mode cursor all read it.

The walkmesh stores wall geometry as many short edges (Endar Spire corridors are ~0.7-1m per edge). Adjacent collinear edges are clustered into "surfaces" at area-load time, so consumers can reason about one physical wall instead of its fragments.

Cluster rule: two edges merge if they share an endpoint (within kEndpointTolMeters) AND their direction vectors agree within kSurfaceCollinearityCosThreshold. Cross-room merging is intentional — KOTOR's "room" is an internal .lyt segmentation and a physical wall regularly spans multiple rooms. The geometric tests decide alone.

Each clustered surface is also reduced to a single straight segment (two extreme endpoints + unit direction + length) so room_topology doesn't have to re-merge the edge soup.

## Declarations (in source order)

- L56 `bool EdgesAreSameSurface(const acc::engine::WallEdge& e1, const acc::engine::WallEdge& e2)`: Returns true if the two edges should belong to the same wall surface: they share an endpoint (within kEndpointTolMeters) AND their direction vectors are roughly collinear (cosine within kSurfaceCollinearityCosThreshold).
- L88 `int UfFind(int i)`
- L96 `void UfUnion(int i, int j)`
- L116 `void ClusterEdgesIntoSurfaces()`: Cluster the cached wall edges (g_walls[0..g_wall_count-1]) into connected-and-collinear surfaces.
- L180 `void BuildSurfaceDescriptors()`
- L371 `void Tick()`
- L383 `bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount)`
- L390 `void Clear()`
- L396 `void RebuildForArea(void* area)`
- L437 `const acc::engine::WallEdge* GetWallBuffer()`
- L441 `const int* GetEdgeSurfaceIdBuffer()`
- L445 `int GetWallCount()`
- L449 `int GetSurfaceCount()`
- L453 `bool GetSurfaceDesc(int idx, WallSurfaceDesc& outDesc)`
- L461 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`
- L513 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`
