# room_topology_shape.cpp (621 lines)

Source: `patches/Accessibility/room_topology_shape.cpp`

The shape of nodes and regions: per-node features from wall probes, the centroid axis of a region, and the pass that gives every region its kind (dead end, corridor, junction, room, open area).

## Declarations (in source order)

- L79 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`: Per-node degree from the snapshot.
- L96 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`: Per-node shape features from the 8-ray clearance probe.
- L261 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`: Long-axis test for an area cluster: is the space clearly elongated, and along which axis word?
- L314 `int AxisOctantMask(acc::strings::Id axisId)`: The two opposite octant bits an elongation axis covers, or 0 for a non-axis Id.
- L345 `ClusterGeom MeasureCluster(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, const BuildPasses& bp)`
- L382 `bool MedianAxisExtents(int n, int root, const BuildPasses& bp, float axisExt[4])`: Per-axis extent of the cluster = median over its members' own 8-ray extents.
- L418 `void CollectExternalExits(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, ClusterExits& ex)`
- L482 `int AreaHint(const ClusterGeom& c, int externalCount, int root)`: How the cluster announces: 0 = by its exit topology (junction, corridor, dead end), 1 = room, 2 = big area.
- L504 `int CollectTransitions(int n, int root, int trigIdxs[kMaxClusterTransitions])`
- L517 `bool TerminalDeadEndAgrees(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root, const BuildPasses& bp)`: The dead-end verdict for the cluster, from its degree-1 members (where a terminal wall would be), not the centroid, which for a multi-node run sits mid-corridor.
- L532 `float Footprint(float extent, const float* axisExt)`: The floor footprint for announce timing (ClusterInfo::footprintM).
- L544 `void Tally(int kind, BuildPasses& bp)`
- L560 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: For each cluster root (the smallest node id in its union-find class): measure it, collect its external exits, classify it via ClassifyCluster, and write the result to every member.
