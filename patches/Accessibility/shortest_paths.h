// Shortest walks over a small weighted graph (Dijkstra), used for the region
// graph of the exit memory, and walking distances as they are spoken.
// Engine-free, so the unit tests cover it.

#pragma once

#include <utility>
#include <vector>

namespace acc::paths {

// adj[i]: the neighbours of node i, each with the length of the edge.
using Adjacency = std::vector<std::vector<std::pair<int, float>>>;

// The distance of a node no walk reaches.
constexpr float kUnreachable = 1e30f;

// From `start`: dist[i], the length of the shortest walk to node i
// (kUnreachable if there is none), and firstHop[i], the neighbour of start
// that walk begins with (-1 for start itself and for unreachable nodes).
// When asked for, prev[i] is the node that walk reaches i from (-1 likewise).
void ShortestPaths(const Adjacency& adj, int start, std::vector<float>& dist,
                   std::vector<int>& firstHop, std::vector<int>* prev = nullptr);

// A walking distance as spoken: rounded to whole steps of `step` metres,
// never less than one step.
int RoundWalk(float metres, int step);

}  // namespace acc::paths
