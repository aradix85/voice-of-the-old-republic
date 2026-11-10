# guidance_pathfind.h (86 lines)

Source: `patches/Accessibility/guidance_pathfind.h`

A* pathfinder over the per-area static nav graph.

Reads CSWSArea pure data — no engine re-entry. We re-solve because the engine refuses to plot a path for the PC (project_addmovetopoint_ leader_broken). Output feeds guidance_description and autowalk.

CSWSArea nav graph (offsets at the bottom of this header):

PathPoint (16 bytes): Vector position + uint32 csr_offset. CSR adjacency: node N's neighbours live at path_connections[meta_N .. meta_{N+1}-1] where meta_K = path_points[K].csr_offset; last node's upper bound is path_connections_count. Edges are symmetric undirected.

First-cut scope: - Single area only — cross-area routing is deferred; the user re-fires Ctrl+- after walking through a transition. - engine_navgraph::kMaxNodes (512) nodes max — open/closed sets are fixed-size arrays.

## Declarations (in source order)

- L54 `enum class PathFailure`: Computes shortest walkmesh-respecting path.
- L64 `bool ComputePath(void* area, const Vector& start, const Vector& goal, std::vector<Vector>& outWaypoints, PathFailure* failure = nullptr)`: Returns: true — non-empty path.

Also 7 constants or variables at namespace scope.
