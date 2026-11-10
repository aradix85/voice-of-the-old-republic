# room_topology_shape.cpp (652 lines)

Source: `patches/Accessibility/room_topology_shape.cpp`

The shape of nodes and regions: per-node features from wall probes, the centroid axis of a region, and the pass that gives every region its kind (dead end, corridor, junction, room, open area).

## Declarations (in source order)

- L79 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`: Per-node degree from the snapshot.
- L96 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`: Per-node shape features from the 8-ray clearance probe.
- L261 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`: Long-axis test for an area cluster: is the space clearly elongated, and along which axis word?
- L314 `int AxisOctantMask(acc::strings::Id axisId)`: The two opposite octant bits an elongation axis covers, or 0 for a non-axis Id.
- L328 `void PassClassifyClusters(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n, BuildPasses& bp)`: ===== Pass 3: per-cluster classification ===== For each cluster root (the smallest node id in its union-find class), compute the centroid, collect external neighbours (dedup by node id), classify via ClassifyCluster, ...
