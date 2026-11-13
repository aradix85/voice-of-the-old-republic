// shortest_paths: the region graph's walks and the spoken walking distance.

#include <vector>

#include "doctest.h"
#include "shortest_paths.h"

using acc::paths::Adjacency;
using acc::paths::kUnreachable;

namespace {

// 0 -1- 1 -1- 2, a direct 0 -5- 2, and node 3 joined to nothing.
Adjacency SmallGraph() {
    Adjacency a(4);
    auto edge = [&](int u, int v, float w) {
        a[u].push_back({v, w});
        a[v].push_back({u, w});
    };
    edge(0, 1, 1.0f);
    edge(1, 2, 1.0f);
    edge(0, 2, 5.0f);
    return a;
}

}  // namespace

TEST_CASE("ShortestPaths takes the shorter way round") {
    std::vector<float> dist;
    std::vector<int>   hop;
    acc::paths::ShortestPaths(SmallGraph(), 0, dist, hop);
    CHECK(dist[0] == 0.0f);
    CHECK(dist[1] == doctest::Approx(1.0f));
    CHECK(dist[2] == doctest::Approx(2.0f));
    CHECK(hop[0] == -1);
    CHECK(hop[1] == 1);
    CHECK(hop[2] == 1);  // via node 1, not the direct 5 m edge
}

TEST_CASE("ShortestPaths gives the node before each on its walk") {
    std::vector<float> dist;
    std::vector<int>   hop;
    std::vector<int>   prev;
    acc::paths::ShortestPaths(SmallGraph(), 0, dist, hop, &prev);
    CHECK(prev[0] == -1);
    CHECK(prev[1] == 0);
    CHECK(prev[2] == 1);  // via node 1, not straight from the start
    CHECK(prev[3] == -1);
}

TEST_CASE("ShortestPaths marks nodes no walk reaches") {
    std::vector<float> dist;
    std::vector<int>   hop;
    acc::paths::ShortestPaths(SmallGraph(), 0, dist, hop);
    CHECK(dist[3] == kUnreachable);
    CHECK(hop[3] == -1);
}

TEST_CASE("ShortestPaths with a start outside the graph reaches nothing") {
    std::vector<float> dist;
    std::vector<int>   hop;
    acc::paths::ShortestPaths(SmallGraph(), 7, dist, hop);
    REQUIRE(dist.size() == 4);
    for (float d : dist) CHECK(d == kUnreachable);
}

TEST_CASE("RoundWalk rounds to whole steps, at least one") {
    CHECK(acc::paths::RoundWalk(0.0f, 5) == 5);
    CHECK(acc::paths::RoundWalk(2.4f, 5) == 5);
    CHECK(acc::paths::RoundWalk(7.4f, 5) == 5);
    CHECK(acc::paths::RoundWalk(7.5f, 5) == 10);
    CHECK(acc::paths::RoundWalk(12.6f, 5) == 15);
    CHECK(acc::paths::RoundWalk(100.0f, 5) == 100);
}
