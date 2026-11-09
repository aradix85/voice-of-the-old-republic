# room_topology_diag.cpp (367 lines)

Source: `patches/Accessibility/room_topology_diag.cpp`

Diagnostic log output of the region graph: nav edges that cross walls, topology metrics, which nodes of a region touch, and the door snapshot.

## Declarations (in source order)

- L13 `void LogDoorSnapshotDetails(void* area)`: Per-door diagnostic dump.
- L118 `void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g)`: Diagnostic: nav-graph vs wall-cache crossing check.
- L229 `void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`: Diagnostic-only: dump per-node topology metrics so we can pick a principled gate later.
- L310 `void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g, int n)`: Diagnostic-only: per multi-node cluster, dump member adjacency.
