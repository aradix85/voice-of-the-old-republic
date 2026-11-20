# guidance_pathfind.cpp (511 lines)

Source: `patches/Accessibility/guidance_pathfind.cpp`

(From guidance_pathfind.h.) A* pathfinder over the per-area static nav graph.

Reads CSWSArea pure data — no engine re-entry. We re-solve because the engine refuses to plot a path for the PC (project_addmovetopoint_ leader_broken). Output feeds guidance_description and autowalk.

CSWSArea nav graph (offsets at the bottom of this header):

PathPoint (16 bytes): Vector position + uint32 csr_offset. CSR adjacency: node N's neighbours live at path_connections[meta_N .. meta_{N+1}-1] where meta_K = path_points[K].csr_offset; last node's upper bound is path_connections_count. Edges are symmetric undirected.

First-cut scope: - Single area only — cross-area routing is deferred; the user re-fires Ctrl+- after walking through a transition. - engine_navgraph::kMaxNodes (512) nodes max — open/closed sets are fixed-size arrays.

## Declarations (in source order)

- L42 `int FindAnchorNode(const acc::engine::navgraph::NavGraphSnapshot& graph, const acc::engine::WallEdge* walls, int wallCount, const Vector& target, const char* role, const bool* allowed = nullptr)`: Pick the path_point a route should start / end on for `target`.
- L105 `int FindBlockingEdge(const acc::engine::WallEdge* walls, int wallCount, const Vector& a, const Vector& b, float& outT)`: Diagnostic: find the closest (smallest-t along a→b) walkmesh edge that blocks the segment a→b.
- L136 `bool ClearOfZones(const std::vector<acc::engine::WallEdge>& zoneWalls, const Vector& a, const Vector& b)`: True when a→b crosses no outline of a turn-back zone (turnback_zones.h).
- L145 `void MarkReachable(const acc::engine::navgraph::NavGraphSnapshot& graph, const std::vector<acc::engine::WallEdge>& zoneWalls, int from, bool* reached)`: Marks the nodes that links clear of the zones reach from `from`.
- L172 `bool ComputePath(void* area, const Vector& start, const Vector& goal, std::vector<Vector>& outWaypoints, PathFailure* failure)`
