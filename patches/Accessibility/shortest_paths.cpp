#include "shortest_paths.h"

namespace acc::paths {

void ShortestPaths(const Adjacency& adj, int start, std::vector<float>& dist,
                   std::vector<int>& firstHop, std::vector<int>* prev) {
    const int n = static_cast<int>(adj.size());
    dist.assign(n, kUnreachable);
    firstHop.assign(n, -1);
    if (prev) prev->assign(n, -1);
    if (start < 0 || start >= n) return;
    std::vector<bool> done(n, false);
    dist[start] = 0.0f;
    for (;;) {
        int   u    = -1;
        float best = kUnreachable;
        for (int i = 0; i < n; ++i) {
            if (!done[i] && dist[i] < best) {
                best = dist[i];
                u    = i;
            }
        }
        if (u < 0) break;
        done[u] = true;
        for (const auto& edge : adj[u]) {
            const int   v  = edge.first;
            const float nd = dist[u] + edge.second;
            if (nd < dist[v]) {
                dist[v]     = nd;
                firstHop[v] = (u == start) ? v : firstHop[u];
                if (prev) (*prev)[v] = u;
            }
        }
    }
}

int RoundWalk(float metres, int step) {
    const int r = static_cast<int>(metres / static_cast<float>(step) + 0.5f) * step;
    return r < step ? step : r;
}

}  // namespace acc::paths
