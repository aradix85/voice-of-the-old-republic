# walkmesh_geometry.cpp (203 lines)

Source: `patches/Accessibility/walkmesh_geometry.cpp`

Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

## Declarations (in source order)

- L18 `bool Same3D(const Vector& p, const Vector& q)`
- L19 `bool SameXY(const Vector& p, const Vector& q)`
- L23 `bool SameEdge(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Same same)`
- L27 `bool SharesEndpoint3D(const WallEdge& p, const WallEdge& q)`
- L32 `float Cross(const Vector& a, const Vector& b, const Vector& p)`: Twice the signed floor-plan area of a-b-p: positive when p lies left of a→b.
- L38 `Vector NearestOnSegmentXY(const Vector& p, const Vector& a, const Vector& b)`: The point of the segment a-b nearest to p on the floor plan, at the segment's height there.
- L52 `Vector NearestOnTriangleXY(const Vector& p, const FloorTri& t)`: The point of the triangle nearest to p on the floor plan, at the triangle's height there: p itself when it lies inside.
- L72 `bool IsWalkableMaterial(int materialId)`
- L81 `bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outPoint)`
- L100 `bool FloorSpotNear(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outSpot)`
- L114 `bool OverWalkableFloor(const FloorTri* tris, int triCount, const Vector& p)`
- L125 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`
- L139 `int DropSameRoomDuplicates(WallEdge* walls, int count)`
- L157 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ)`
