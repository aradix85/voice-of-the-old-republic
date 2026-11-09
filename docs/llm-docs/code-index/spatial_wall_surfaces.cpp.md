# spatial_wall_surfaces.cpp (548 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.cpp`

(From spatial_wall_surfaces.h.) Wall cache + surface clustering.

The walkmesh stores wall geometry as many short edges (Endar Spire corridors are ~0.7-1m per edge). For change-detection purposes that produces permanent chatter as the player walks parallel to a wall: each edge's closest-point distance jumps when the player passes its endpoint, even though the physical wall surface hasn't changed distance. We solve this by clustering adjacent collinear edges into "surfaces" at area-load time, so consumers can track distance per surface rather than per edge.

Cluster rule: two edges merge if they share an endpoint (within kEndpointTolMeters) AND their direction vectors agree within kSurfaceCollinearityCosThreshold. Cross-room merging is intentional — KOTOR's "room" is an internal .lyt segmentation and a physical wall regularly spans multiple rooms. The geometric tests decide alone.

Each clustered surface is also reduced to a single straight segment (two extreme endpoints + unit direction + length) so room_topology doesn't have to re-merge the edge soup.

## Declarations (in source order)

- L55 `bool EdgesAreSameSurface(const acc::engine::WallEdge& e1, const acc::engine::WallEdge& e2)`: Returns true if the two edges should belong to the same wall surface: they share an endpoint (within kEndpointTolMeters) AND their direction vectors are roughly collinear (cosine within kSurfaceCollinearityCosThreshold).
- L87 `int UfFind(int i)`
- L95 `void UfUnion(int i, int j)`
- L115 `void ClusterEdgesIntoSurfaces()`: Cluster the cached wall edges (g_walls[0..g_wall_count-1]) into connected-and-collinear surfaces.
- L179 `void BuildSurfaceDescriptors()`
- L367 `void Clear()`
- L373 `void RebuildForArea(void* area)`
- L414 `const acc::engine::WallEdge* GetWallBuffer()`
- L418 `const int* GetEdgeSurfaceIdBuffer()`
- L422 `int GetWallCount()`
- L426 `int GetSurfaceCount()`
- L430 `bool GetSurfaceDesc(int idx, WallSurfaceDesc& outDesc)`
- L438 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`
- L490 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`
