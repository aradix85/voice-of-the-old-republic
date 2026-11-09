# engine_navgraph.h (49 lines)

Source: `patches/Accessibility/engine_navgraph.h`

CSWSArea nav-graph reader — shared snapshot helpers.

Pure read, SEH-guarded. Used by guidance_pathfind and room_topology (the offset constants live in guidance_pathfind.h — one canonical location).

PathPoint (16 bytes): Vector position + uint32 csr_offset. CSR adjacency: neighbours of node N at path_connections[meta_N .. meta_{N+1}-1] where meta_K = path_points[K].csr_offset; last node's upper bound is path_connections_count. Edges are symmetric undirected.

## Declarations (in source order)

- L22 `struct PathPointSnapshot`
- L29 `struct NavGraphSnapshot`: Neighbour run for node N: conns[nodes[N].csrOffset ..
- L41 `bool SnapshotNavGraph(void* area, NavGraphSnapshot& out)`: True on non-empty snapshot.
- L45 `void NeighbourRange(const NavGraphSnapshot& g, int node, int& outLo, int& outHi)`: Bounds-checked against malformed CSR offsets — returns empty range rather than indexing out of bounds.

Also 2 constants or variables at namespace scope.
