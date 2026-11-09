# room_topology.cpp (580 lines)

Source: `patches/Accessibility/room_topology.cpp`

Wall-topology decomposition — see room_topology.h for goal + algorithm.

EXPERIMENTAL — alternative-direction-calculation-system branch.

Path 3 (2026-05-13) consumes the engine's per-area nav graph (the same path_points / path_connections fields the engine itself uses for AI pathfinding) and classifies each node by its CSR-adjacency degree: - degree 1               → dead end pointing at its single neighbour - degree 2, same room    → corridor (axis defined by both neighbours) - degree 2, cross-room   → transition (doorway between two rooms) - degree ≥ 3             → junction (comma-separated octant list)

Previous revisions (face-graph flood-fill, parallel-pair sweep) all failed empirically on K1's authoring style — see commit history on this branch. The nav graph is what BioWare ships, and the engine itself trusts it for path solving, so we're operating on the same ground truth the level designers were aware of.

## Declarations (in source order)

- L68 `void Reset()`
- L83 `bool HasGraphForArea(void* area)`
- L89 `void ResetNavGraphRetry()`
- L93 `void BuildForArea(void* area)`
- L299 `void DumpGraphToLog()`
- L315 `bool GetClusterInfo(void* area, int clusterId, ClusterInfo& out)`
- L331 `bool ListClusters(void* area, std::vector<ClusterSummary>& out)`
- L351 `bool GetClusterLinks(void* area, std::vector<ClusterLink>& out)`
- L358 `bool LookupAt(void* area, const Vector& worldPos, std::string& outLabel, int& outSig, int& outClusterId, bool allowDiagLog, bool requireWallReachable, bool zTrusted, std::vector<ExitMark>* outExits)`
