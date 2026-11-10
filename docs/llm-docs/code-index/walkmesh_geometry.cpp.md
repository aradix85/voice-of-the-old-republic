# walkmesh_geometry.cpp (106 lines)

Source: `patches/Accessibility/walkmesh_geometry.cpp`

Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

## Declarations (in source order)

- L13 `bool Same3D(const Vector& p, const Vector& q)`
- L14 `bool SameXY(const Vector& p, const Vector& q)`
- L18 `bool SameEdge(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Same same)`
- L22 `bool SharesEndpoint3D(const WallEdge& p, const WallEdge& q)`
- L28 `int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount)`
- L42 `int DropSameRoomDuplicates(WallEdge* walls, int count)`
- L60 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ)`
