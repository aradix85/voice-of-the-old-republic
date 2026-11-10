# room_topology_classify.cpp (317 lines)

Source: `patches/Accessibility/room_topology_classify.cpp`

Pass 3 of the build: gives every cluster its kind (dead end, corridor, junction, room, area) and label, from its members' shape flags, its footprint and its external exits, and writes the result to every member.

## Declarations (in source order)

- L41 `ClusterGeom MeasureCluster(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, const BuildPasses& bp)`
- L78 `bool MedianAxisExtents(int n, int root, const BuildPasses& bp, float axisExt[4])`: Per-axis extent of the cluster = median over its members' own 8-ray extents.
- L114 `void CollectExternalExits(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, ClusterExits& ex)`
- L178 `int AreaHint(const ClusterGeom& c, int externalCount, int root)`: How the cluster announces: 0 = by its exit topology (junction, corridor, dead end), 1 = room, 2 = big area.
- L200 `int CollectTransitions(int n, int root, int trigIdxs[kMaxClusterTransitions])`
- L213 `bool TerminalDeadEndAgrees(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, const BuildPasses& bp)`: The dead-end verdict for the cluster, from its degree-1 members (where a terminal wall would be), not the centroid, which for a multi-node run sits mid-corridor.
- L228 `float Footprint(float extent, const float* axisExt)`: The floor footprint for announce timing (ClusterInfo::footprintM).
- L240 `void Tally(int kind, BuildPasses& bp)`
- L256 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: For each cluster root (the smallest node id in its union-find class): measure it, collect its external exits, classify it via ClassifyCluster, and write the result to every member.
