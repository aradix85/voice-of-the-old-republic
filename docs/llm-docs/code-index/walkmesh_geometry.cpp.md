# walkmesh_geometry.cpp (231 lines)

Source: `patches/Accessibility/walkmesh_geometry.cpp`

Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

## Declarations (in source order)

- L20 `bool Same3D(const Vector& p, const Vector& q)`
- L21 `bool SameXY(const Vector& p, const Vector& q)`
- L25 `bool SameEdge(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Same same)`
- L29 `bool SharesEndpoint3D(const WallEdge& p, const WallEdge& q)`
- L34 `float Cross(const Vector& a, const Vector& b, const Vector& p)`: Twice the signed floor-plan area of a-b-p: positive when p lies left of a→b.
- L40 `Vector NearestOnSegmentXY(const Vector& p, const Vector& a, const Vector& b)`: The point of the segment a-b nearest to p on the floor plan, at the segment's height there.
- L54 `Vector NearestOnTriangleXY(const Vector& p, const FloorTri& t)`: The point of the triangle nearest to p on the floor plan, at the triangle's height there: p itself when it lies inside.
- L74 `bool IsWalkableMaterial(int materialId)`
- L83 `bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outPoint)`
- L102 `bool FloorSpotNear(const FloorTri* tris, int triCount, const Vector& p, float maxM, Vector& outSpot)`
- L116 `bool OverWalkableFloor(const FloorTri* tris, int triCount, const Vector& p)`
- L127 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`
- L141 `int DropSameRoomDuplicates(WallEdge* walls, int count)`
- L159 `bool PointInPolygonXY(const Vector* poly, int count, float x, float y)`
- L173 `float DistToPolygonXY(const Vector* poly, int count, float x, float y)`
- L185 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ)`
