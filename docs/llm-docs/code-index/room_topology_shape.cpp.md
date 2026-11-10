# room_topology_shape.cpp (220 lines)

Source: `patches/Accessibility/room_topology_shape.cpp`

The shape of nodes and regions: per-node features from the 8-ray wall probe (open, room, floor height, dead-end verdict, axis extents), the long axis of a region, and the octants an axis word covers.

## Declarations (in source order)

- L37 `const char* ShapeClass(int openRays, int longBits, bool room)`: The tentative per-node class, for the clearance log only: the connector classes count real long arms (open-scale rays), rooms go by evenness.
- L55 `void LogAxisVerdict(const Vector& centroid, const float ext[4], bool fromMembers, bool elong, int longA)`
- L66 `int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node)`
- L75 `void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g, int n, bool* nodeOpen, bool* nodeRoom, float* nodeFloorZ, bool* nodeDeadEnd, float (*nodeAxisExt)[4])`: Runs once at the top of BuildForArea, before any merge pass, so every pass sees the same flags; every ray and feature is logged so the thresholds stay re-pickable.
- L173 `bool ComputeCentroidAxis(const Vector& centroid, float floorZ, const float* memberExt, acc::strings::Id& outAxisId)`: memberExt holds the cluster's per-axis extents, each the median over its members' probes.
- L210 `int AxisOctantMask(acc::strings::Id axisId)`: The two opposite octant bits an axis word covers, or 0.
