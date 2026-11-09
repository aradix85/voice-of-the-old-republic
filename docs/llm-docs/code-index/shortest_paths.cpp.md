# shortest_paths.cpp (41 lines)

Source: `patches/Accessibility/shortest_paths.cpp`

(From shortest_paths.h.) Shortest walks over a small weighted graph (Dijkstra), used for the region graph of the exit memory, and walking distances as they are spoken. Engine-free, so the unit tests cover it.

## Declarations (in source order)

- L5 `void ShortestPaths(const Adjacency& adj, int start, std::vector<float>& dist, std::vector<int>& firstHop)`
- L35 `int RoundWalk(float metres, int step)`
