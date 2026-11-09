# spatial_wall_surfaces.h (107 lines)

Source: `patches/Accessibility/spatial_wall_surfaces.h`

Wall cache + surface clustering.

The walkmesh stores wall geometry as many short edges (Endar Spire corridors are ~0.7-1m per edge). For change-detection purposes that produces permanent chatter as the player walks parallel to a wall: each edge's closest-point distance jumps when the player passes its endpoint, even though the physical wall surface hasn't changed distance. We solve this by clustering adjacent collinear edges into "surfaces" at area-load time, so consumers can track distance per surface rather than per edge.

Cluster rule: two edges merge if they share an endpoint (within kEndpointTolMeters) AND their direction vectors agree within kSurfaceCollinearityCosThreshold. Cross-room merging is intentional — KOTOR's "room" is an internal .lyt segmentation and a physical wall regularly spans multiple rooms. The geometric tests decide alone.

Each clustered surface is also reduced to a single straight segment (two extreme endpoints + unit direction + length) so room_topology doesn't have to re-merge the edge soup.

## Declarations (in source order)

- L44 `struct WallSurfaceDesc`: One wall surface = chain of collinear endpoint-sharing edges clustered at area-load via union-find.
- L57 `void RebuildForArea(void* area)`: Rebuild on area change.
- L61 `void Clear()`: Reset to "no area" state.
- L66 `const acc::engine::WallEdge* GetWallBuffer()`: Borrowed pointer into static storage, valid until the next RebuildForArea or Clear.
- L72 `const int* GetEdgeSurfaceIdBuffer()`: Parallel array to GetWallBuffer(): element i is the surface index for wall edge i.
- L75 `int GetWallCount()`: 0 before first RebuildForArea.
- L78 `int GetSurfaceCount()`: 0 before first RebuildForArea.
- L83 `bool GetSurfaceDesc(int idx, WallSurfaceDesc& outDesc)`: False on out-of-range, before clustering runs, or for surfaces that couldn't reduce to a single straight segment (closed-loop / branching edge cases flagged with edge_count == 0).
- L89 `bool SegmentCrossesSurface(const Vector& a, const Vector& b, Vector& outHitPoint)`: 2D segment-vs-surface intersection.
- L104 `int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut)`: Floor heights of the walkmesh triangles containing (x,y), one entry per stacked storey (bridge over tunnel), exact on ramps via plane interpolation.

Also 3 constants or variables at namespace scope.
