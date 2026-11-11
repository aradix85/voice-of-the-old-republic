# engine_navgraph.cpp (161 lines)

Source: `patches/Accessibility/engine_navgraph.cpp`

CSWSArea nav-graph reader; engine_navgraph.h has the design.

## Declarations (in source order)

- L27 `bool ReadMeta(void* area, GraphMeta& out)`: Snapshot the nav graph metadata.
- L53 `int LoadPoints(const GraphMeta& meta, std::vector<PathPointSnapshot>& out)`: Read every path_points entry into the snapshot vector.
- L92 `int LoadConnections(const GraphMeta& meta, std::vector<uint32_t>& out)`: Read the flat connections array.
- L124 `bool SnapshotNavGraph(void* area, NavGraphSnapshot& out)`
- L141 `void NeighbourRange(const NavGraphSnapshot& g, int node, int& outLo, int& outHi)`
