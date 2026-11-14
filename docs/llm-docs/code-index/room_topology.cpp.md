# room_topology.cpp (594 lines)

Source: `patches/Accessibility/room_topology.cpp`

The region graph of an area (see room_topology.h for its purpose). It is built from the engine's per-area nav graph (the same path_points / path_connections fields the engine itself uses for AI pathfinding); each node is classified by its CSR-adjacency degree: - degree 1               → dead end pointing at its single neighbour - degree 2, same room    → corridor (axis defined by both neighbours) - degree 2, cross-room   → transition (doorway between two rooms) - degree ≥ 3             → junction (comma-separated octant list)

The nav graph is what BioWare ships and the engine trusts for path solving, the same ground truth the level designers worked with. Nodes are then merged into regions and each region labelled. This file holds the pipeline (BuildForArea), Reset and the public queries.

## Declarations (in source order)

- L77 `void Reset()`
- L92 `bool HasGraphForArea(void* area)`
- L98 `void ResetNavGraphRetry()`
- L107 `void BuildClusterLinks(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`: Region adjacency for the exploration overview: which regions share a nav edge that does not cross a wall, and whether every such edge runs through a door.
- L144 `void BuildForArea(void* area)`
- L316 `void DumpGraphToLog()`
- L332 `bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out)`
- L348 `bool NearestClusterNode(void* area, int clusterId, const Vector& p, Vector& out)`
- L362 `bool ListClusters(void* area, std::vector<ClusterSummary>& out)`
- L382 `bool GetClusterLinks(void* area, std::vector<ClusterLink>& out)`
- L415 `NodeScan ScanNodes(const Vector& worldPos, bool requireWallReachable, bool zTrusted, const Vector* closedDoor)`: A node is reachable when it is on the caller's floor (zTrusted: its calibrated floor within the wall test's own z tolerance, so "same floor" means one thing in both filters) and no wall lies between (requireWallReacha...
- L471 `void LogScan(const NodeScan& s, const Vector& worldPos)`: Logged only when the filter rejected a candidate nearer than the pick, or rejected every one (a sign the wall cache or the position is wrong).
- L509 `bool InSnapRange(int node, float distSq)`
- L516 `bool ClosedDoorNear(void* area, const Vector& p, Vector& outDoor)`: The closed door within kClosedDoorNearM of `p`, if any.
- L529 `bool LookupAt(void* area, const Vector& worldPos, std::string& outLabel, int& outSig, int& outClusterId, bool allowDiagLog, bool requireWallReachable, bool zTrusted, std::vector<ExitMark>* outExits)`
