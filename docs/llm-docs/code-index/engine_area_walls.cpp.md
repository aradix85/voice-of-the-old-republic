# engine_area_walls.cpp (188 lines)

Source: `patches/Accessibility/engine_area_walls.cpp`

Walkmesh extraction: reads each room's collision mesh from CSWSArea's room list, takes its faces to world space, and builds the area's wall cache (perimeter edges minus portals and duplicates, filtered in walkmesh_geometry.cpp) and its floor triangle cache.

## Declarations (in source order)

- L45 `bool ReadRoomMesh(void* surfaceMesh, RoomMesh* m)`
- L63 `void LocalToWorld(void* surfaceMesh, Vector* p)`: CCollisionMesh::LocalToWorld.
- L74 `bool ReadFace(const RoomMesh& m, uint32_t f, Face* out)`: False on a fault: the face is skipped.
- L92 `void ForEachRoomMesh(void* area, Fn fn)`
- L113 `void ForEachSide(const RoomMesh& m, Fn fn)`
- L126 `int CollectTriSides(void* area)`: Fills s_triSides; returns how many it holds.
- L145 `int BuildAreaWallCache(void* area, WallEdge* outBuf, int maxEdges)`
- L171 `int BuildAreaFloorCache(void* area, FloorTri* outBuf, int maxTris)`
