# walkmesh_geometry.h (75 lines)

Source: `patches/Accessibility/walkmesh_geometry.h`

Engine-free geometry over the walkmesh wall cache: the wall edge and floor triangle records engine_area_walls.cpp extracts, and the segment-vs-wall test the guidance, topology and cursor code use.

## Declarations (in source order)

- L16 `struct WallEdge`: One perimeter walkmesh edge — a triangle side that has no neighbour (`SurfaceMeshAdjacency.indices[e] == -1`) and therefore corresponds to a physical wall in the room.
- L28 `struct FloorTri`: One walkmesh floor triangle, all three corners in WORLD space (LocalToWorld applied at extraction, same as WallEdge).
- L67 `bool SegmentCrossesWalkmesh(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b, Vector& outHitPoint, bool ignoreZ = false)`: Segment-vs-perimeter test in the XY plane, with an optional 3D guard: a wall is only a blocker when its edge sits within kWallCrossZToleranceM of the ray at the crossing point.

Also 1 constants or variables at namespace scope.
