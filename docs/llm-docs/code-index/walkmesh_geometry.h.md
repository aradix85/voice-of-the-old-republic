# walkmesh_geometry.h (106 lines)

Source: `patches/Accessibility/walkmesh_geometry.h`

Engine-free geometry over the walkmesh wall cache: the wall edge and floor triangle records engine_area_walls.cpp extracts, the two filters that turn its raw perimeter edges into real walls, and the segment-vs-wall test the guidance, topology and cursor code use.

## Declarations (in source order)

- L14 `struct WallEdge`: One perimeter walkmesh edge: a triangle side with no neighbour (SurfaceMeshAdjacency index -1), in world space.
- L24 `struct FloorTri`: One walkmesh floor triangle in world space.
- L34 `bool IsWalkableMaterial(int materialId)`: True for a surfacemat.2da row a creature can stand on (its "walk" column; the same rows in both games, bar KOTOR 2's walkable BottomlessPit, which nothing here should send anyone onto).
- L40 `bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outPoint)`: The point of the walkable floor nearest to p on the floor plan, within maxM and within kWallCrossZToleranceM of p's height: p itself when it stands on that floor, else a point on the floor's edge.
- L48 `bool FloorSpotNear(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outSpot)`: Where to stand to be as near to p as the walkable floor lets anyone come: a step onto the floor from its edge nearest p, straight away from p, or that edge itself where the floor is narrower than the step.
- L54 `bool OverWalkableFloor(const FloorTri* tris, int triCount, const Vector& p)`: True when a walkable face lies under p on the floor plan, at whatever height: for a point whose own height is a guess (the hops of a route walk carry none).
- L57 `struct TriEdge`: Any triangle side of a room, for the portal filter.
- L69 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`: Drops each wall edge that coincides (about 1 cm, either direction) with a triangle side of ANOTHER room.
- L76 `int DropSameRoomDuplicates(WallEdge* walls, int count)`: Drops the second of each same-room pair whose floor-plan footprints match and that share at least one 3D endpoint: one edge on two non-manifold faces, or a step's flat foot edge plus its slanted face edge (they share ...
- L80 `bool PointInPolygonXY(const Vector* poly, int count, float x, float y)`: True when (x, y) lies inside the polygon on the floor plan (even-odd rule).
- L84 `float DistToPolygonXY(const Vector* poly, int count, float x, float y)`: 0 inside the polygon, else the floor-plan distance to its outline; 1e9 for fewer than three corners.
- L98 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ = false)`: The closest wall that a→b crosses in the floor plan (smallest t in [0,1]), whose z at the crossing is within kWallCrossZToleranceM of the ray's.

Also 1 constants or variables at namespace scope.
