# engine_area_walls.cpp (583 lines)

Source: `patches/Accessibility/engine_area_walls.cpp`

walkmesh wall-edge extraction.

Split out of engine_area.cpp by the Phase-1 structure pass (refactoring candidate 3). This is the Pillar-1 geometry foundation: it reads room collision meshes, transforms face edges to world space, and builds the per-area wall cache that room_topology.cpp consumes. Nothing here touches the object model or the map surface - it is pure geometry over CSWSArea's room list.

Declarations stay in engine_area.h - splitting that header is a separate, deferred item, so every existing includer is unaffected.

## Declarations (in source order)

- L37 `void* GetRoomSurfaceMesh(void* room)`: Read a CSWSRoom's surface_mesh pointer.
- L50 `bool ReadFaceVertexIndices(unsigned char* faces, uint32_t f, uint32_t outV[3])`: Read the three uint32 vertex indices for face `f` from a contiguous face-index array.
- L67 `void TransformEdgeEndpoints(void* surfaceMesh, PFN_CollisionMeshLocalToWorld fn, const Vector& localA, const Vector& localB, Vector& outWorldA, Vector& outWorldB)`: Transform a pair of local vertices through the room's CCollisionMesh::LocalToWorld.
- L87 `int ScanRoomWallEdges(void* surfaceMesh, int roomId, WallEdge* outBuf, int maxEdges, int alreadyWritten)`: Walk every triangle of one room's surface mesh, emit a WallEdge for each adjacency==-1 side.
- L190 `int ScanRoomFloorTris(void* surfaceMesh, FloorTri* outBuf, int maxTris, int alreadyWritten)`: Walk every face of one room's surface mesh, emit each as a FloorTri with world-space corners.
- L268 `static int ScanRoomAllTriangleEdges(void* surfaceMesh, int roomId, int alreadyWritten)`: Mirror of ScanRoomWallEdges but emits every triangle edge regardless of adjacency value.
- L326 `int BuildAreaWallCache(void* area, WallEdge* outBuf, int maxEdges)`
- L557 `int BuildAreaFloorCache(void* area, FloorTri* outBuf, int maxTris)`
