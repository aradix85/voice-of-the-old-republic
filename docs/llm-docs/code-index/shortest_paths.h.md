# shortest_paths.h (29 lines)

Source: `patches/Accessibility/shortest_paths.h`

Shortest walks over a small weighted graph (Dijkstra), used for the region graph of the exit memory, and walking distances as they are spoken. Engine-free, so the unit tests cover it.

## Declarations (in source order)

- L13 `using Adjacency`: adj[i]: the neighbours of node i, each with the length of the edge.
- L21 `void ShortestPaths(const Adjacency& adj, int start, std::vector<float>& dist, std::vector<int>& firstHop)`: From `start`: dist[i], the length of the shortest walk to node i (kUnreachable if there is none), and firstHop[i], the neighbour of start that walk begins with (-1 for start itself and for unreachable nodes).
- L26 `int RoundWalk(float metres, int step)`: A walking distance as spoken: rounded to whole steps of `step` metres, never less than one step.

Also 1 constants or variables at namespace scope.
