# room_topology.cpp (579 lines)

Source: `patches/Accessibility/room_topology.cpp`

The region graph of an area (see room_topology.h for its purpose). It is built from the engine's per-area nav graph (the same path_points / path_connections fields the engine itself uses for AI pathfinding); each node is classified by its CSR-adjacency degree: - degree 1               → dead end pointing at its single neighbour - degree 2, same room    → corridor (axis defined by both neighbours) - degree 2, cross-room   → transition (doorway between two rooms) - degree ≥ 3             → junction (comma-separated octant list)

The nav graph is what BioWare ships and the engine trusts for path solving, the same ground truth the level designers worked with. Nodes are then merged into regions and each region labelled. This file holds the pipeline (BuildForArea), Reset and the public queries.

## Declarations (in source order)

- L67 `void Reset()`
- L82 `bool HasGraphForArea(void* area)`
- L88 `void ResetNavGraphRetry()`
- L92 `void BuildForArea(void* area)`
- L298 `void DumpGraphToLog()`
- L314 `bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out)`
- L330 `bool ListClusters(void* area, std::vector<ClusterSummary>& out)`
- L350 `bool GetClusterLinks(void* area, std::vector<ClusterLink>& out)`
- L357 `bool LookupAt(void* area, const Vector& worldPos, std::string& outLabel, int& outSig, int& outClusterId, bool allowDiagLog, bool requireWallReachable, bool zTrusted, std::vector<ExitMark>* outExits)`
