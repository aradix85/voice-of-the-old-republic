# engine_navgraph.h (148 lines)

Source: `patches/Accessibility/engine_navgraph.h`

CSWSArea nav-graph reader — shared snapshot helpers.

Pure read, SEH-guarded. Used by guidance_pathfind and room_topology (the offset constants live in guidance_pathfind.h — one canonical location).

PathPoint (16 bytes): x, y, connection count (+0x08), first connection (+0x0c). Neighbours of node N are path_connections[first_N .. first_N + count_N - 1] — exactly how the engine's successor generator reads them. Authored .pth data stores the runs in point order (all 198 K2 module graphs checked 2026-11-24), but BridgeIslands relocates a bridged point's run to the end of the array, so readers must never derive a run end from the next point. Edges are symmetric undirected.

## Declarations (in source order)

- L24 `struct WallEdge`
- L29 `struct PathPointSnapshot`
- L36 `struct NavGraphSnapshot`: Neighbour run for node N: conns[csrOffset ..
- L48 `bool SnapshotNavGraph(void* area, NavGraphSnapshot& out)`: True on non-empty snapshot.
- L52 `void NeighbourRange(const NavGraphSnapshot& g, int node, int& outLo, int& outHi)`: Bounds-checked against malformed CSR offsets — returns empty range rather than indexing out of bounds.
- L96 `struct IslandBridge`
- L102 `struct IslandBridgeStats`
- L116 `IslandBridgeStats BridgeIslands(NavGraphSnapshot& g, const WallEdge* walls, int wallCount, std::vector<IslandBridge>* outBridges = nullptr)`: Mutates `g` in place: the connection array keeps its authored prefix verbatim and each bridged point's run is re-appended at the end with the new link added (only those points' first/count change).
- L136 `enum class BridgeWrite`: Bridge the ENGINE's own path graph for `area`, so the engine planner (walk-to, use/talk approach, map-pin walks, companions following) and every mod consumer that snapshots it all route over one linked network.
- L144 `BridgeWrite EnsureAreaIslandsBridged(void* area, const WallEdge* walls, int wallCount)`

Also 4 constants or variables at namespace scope.
