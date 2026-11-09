# engine_navgraph.cpp (176 lines)

Source: `patches/Accessibility/engine_navgraph.cpp`

CSWSArea nav-graph reader — see engine_navgraph.h.

Extracted 2026-05-13 from guidance_pathfind.cpp's anon-namespace SnapshotGraphMeta + LoadPoints + LoadConnections helpers. Behaviour is preserved verbatim; the only mechanical changes are the snapshot container (vector replaces static buffers, freeing the caller from kMax* assumptions outside this TU) and the namespace move.

## Declarations (in source order)

- L23 `bool SafeRead(void* base, size_t offset, T& out)`
- L44 `bool ReadMeta(void* area, GraphMeta& out)`: Snapshot the nav graph metadata.
- L68 `int LoadPoints(const GraphMeta& meta, std::vector<PathPointSnapshot>& out)`: Read every path_points entry into the snapshot vector.
- L107 `int LoadConnections(const GraphMeta& meta, std::vector<uint32_t>& out)`: Read the flat connections array.
- L139 `bool SnapshotNavGraph(void* area, NavGraphSnapshot& out)`
- L156 `void NeighbourRange(const NavGraphSnapshot& g, int node, int& outLo, int& outHi)`
