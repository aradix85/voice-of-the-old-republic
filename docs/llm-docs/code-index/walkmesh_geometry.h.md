# walkmesh_geometry.h (72 lines)

Source: `patches/Accessibility/walkmesh_geometry.h`

Engine-free geometry over the walkmesh wall cache: the wall edge and floor triangle records engine_area_walls.cpp extracts, the two filters that turn its raw perimeter edges into real walls, and the segment-vs-wall test the guidance, topology and cursor code use.

## Declarations (in source order)

- L14 `struct WallEdge`: One perimeter walkmesh edge: a triangle side with no neighbour (SurfaceMeshAdjacency index -1), in world space.
- L24 `struct FloorTri`: One walkmesh floor triangle in world space.
- L31 `struct TriEdge`: Any triangle side of a room, for the portal filter.
- L43 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`: Drops each wall edge that coincides (about 1 cm, either direction) with a triangle side of ANOTHER room.
- L50 `int DropSameRoomDuplicates(WallEdge* walls, int count)`: Drops the second of each same-room pair whose floor-plan footprints match and that share at least one 3D endpoint: one edge on two non-manifold faces, or a step's flat foot edge plus its slanted face edge (they share ...
- L64 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ = false)`: The closest wall that a→b crosses in the floor plan (smallest t in [0,1]), whose z at the crossing is within kWallCrossZToleranceM of the ray's.

Also 1 constants or variables at namespace scope.
