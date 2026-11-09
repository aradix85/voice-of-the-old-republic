# room_topology_label.cpp (300 lines)

Source: `patches/Accessibility/room_topology_label.cpp`

The label of a region: decides from the region's shape which kind of label it gets (ClassifyClusterShape) and builds it with its exits (ClassifyCluster).

## Declarations (in source order)

- L13 `void ClassifyClusterShape(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, int trigCount, const std::string& trigEntries,`
- L247 `void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g, const Vector& centroid, const int* externalNbs, const int* externalSrcs, const int* externalDoorIdx, int externalCount, const int* trigIdxs, int trigCount, int areaHint, float centroidFloorZ,`: Classification entry point: pick the shape, then guarantee the cluster ends up with SOME label.
