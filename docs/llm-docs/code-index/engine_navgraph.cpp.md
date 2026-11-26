# engine_navgraph.cpp (406 lines)

Source: `patches/Accessibility/engine_navgraph.cpp`

CSWSArea nav-graph reader; engine_navgraph.h has the design.

## Declarations (in source order)

- L30 `bool ReadMeta(void* area, GraphMeta& out)`: Snapshot the nav graph metadata.
- L56 `int LoadPoints(const GraphMeta& meta, std::vector<PathPointSnapshot>& out)`: Read every path_points entry into the snapshot vector.
- L97 `int LoadConnections(const GraphMeta& meta, std::vector<uint32_t>& out)`: Read the flat connections array.
- L129 `bool SnapshotNavGraph(void* area, NavGraphSnapshot& out)`
- L146 `void NeighbourRange(const NavGraphSnapshot& g, int node, int& outLo, int& outHi)`
- L173 `bool LinkBlocked(const WallEdge* walls, int wallCount, const Vector& a, const Vector& b)`: A link is usable only if a body fits along it: centre line plus two parallel lines kBridgeClearanceM either side must all be wall-clear.
- L194 `IslandBridgeStats BridgeIslands(NavGraphSnapshot& g, const WallEdge* walls, int wallCount, std::vector<IslandBridge>* outBridges)`: No __try in here (C2712: it owns vectors).
- L286 `bool SwapInConnections(void* area, uint32_t expectPoints, uint32_t expectPointsPtr, uint32_t expectConnsPtr, uint32_t* fresh, uint32_t freshCount, const int* idx, const uint32_t* first, const uint32_t* count, int touchedCount)`: Engine write, SEH-guarded, POD-only (C2712).
- L321 `uint32_t* EngineAllocDwords(uint32_t count)`
- L332 `BridgeWrite EnsureAreaIslandsBridged(void* area, const WallEdge* walls, int wallCount)`
