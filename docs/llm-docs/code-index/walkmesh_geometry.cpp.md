# walkmesh_geometry.cpp (173 lines)

Source: `patches/Accessibility/walkmesh_geometry.cpp`

Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

## Declarations (in source order)

- L13 `bool Same3D(const Vector& p, const Vector& q)`
- L14 `bool SameXY(const Vector& p, const Vector& q)`
- L18 `bool SameEdge(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Same same)`
- L22 `bool SharesEndpoint3D(const WallEdge& p, const WallEdge& q)`
- L27 `float Cross(const Vector& a, const Vector& b, const Vector& p)`: Twice the signed floor-plan area of a-b-p: positive when p lies left of a→b.
- L33 `Vector NearestOnSegmentXY(const Vector& p, const Vector& a, const Vector& b)`: The point of the segment a-b nearest to p on the floor plan, at the segment's height there.
- L47 `Vector NearestOnTriangleXY(const Vector& p, const FloorTri& t)`: The point of the triangle nearest to p on the floor plan, at the triangle's height there: p itself when it lies inside.
- L67 `bool IsWalkableMaterial(int materialId)`
- L76 `bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outPoint)`
- L95 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`
- L109 `int DropSameRoomDuplicates(WallEdge* walls, int count)`
- L127 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ)`
