// Diagnostic log output of the region graph: nav edges that cross walls,
// topology metrics, which nodes of a region touch, and the door snapshot.

#include "room_topology_internal.h"

namespace acc::room_topology {

// Per-door diagnostic dump. Position + transition string + geometry
// probe block (nearest nav-graph node, approach direction, .lyt-room
// in front/behind, 4 cardinal walkmesh probes). Requires the nav
// graph to already be built (uses g_graph.node_pos) and the wall
// cache populated (ProbeDistance needs it).
void LogDoorSnapshotDetails(void* area) {
    int withTransition = 0;
    for (int i = 0; i < g_graph.door_count; ++i) {
        if (g_graph.doors[i].transitionDest[0]) ++withTransition;
    }
    acclog::Write("WallTopo",
                  "SnapshotDoors: collected %d doors (%d transition)",
                  g_graph.door_count, withTransition);
    for (int i = 0; i < g_graph.door_count; ++i) {
        const DoorRecord& d = g_graph.doors[i];
        acclog::Write("WallTopo",
                      "  door[%d] pos=(%.1f,%.1f,%.1f) locName=\"%s\" "
                      "transition=\"%s\"",
                      i, d.pos.x, d.pos.y, d.pos.z,
                      d.locName[0] ? d.locName : "(none)",
                      d.transitionDest[0] ? d.transitionDest : "(none)");

        // Diagnostic: what's the geometry around this door? We answer:
        //   1. Which .lyt-room is the door IN (engine resolves point→room)
        //   2. Which .lyt-room sits 2m PAST the door (back side)
        //   3. Walkmesh probe distances on 4 cardinal axes from the
        //      door position (room walkmesh excludes door collision
        //      meshes, so probes can pass through where the door is)
        //
        // The approach direction is estimated from the nearest nav-
        // graph node — the player walks toward the door from a corridor
        // node, so (door - nearestNode) is the "into" direction. The
        // 2m past-the-door point lives along that same axis. Comparing
        // front_room to back_room tells us whether the door has any
        // walkable interior behind it (front != back AND back is
        // valid → stub room exists) or is pure decoration (back == -1
        // or back == front).
        int nearestNode = -1;
        float bestSq = 1e30f;
        for (int n = 0; n < g_graph.node_count; ++n) {
            float ndx = g_graph.node_pos[n].x - d.pos.x;
            float ndy = g_graph.node_pos[n].y - d.pos.y;
            float d2 = ndx * ndx + ndy * ndy;
            if (d2 < bestSq) { bestSq = d2; nearestNode = n; }
        }
        Vector approachVec = {0.0f, 0.0f, 0.0f};
        if (nearestNode >= 0) {
            approachVec.x = d.pos.x - g_graph.node_pos[nearestNode].x;
            approachVec.y = d.pos.y - g_graph.node_pos[nearestNode].y;
            float mag = std::sqrt(approachVec.x * approachVec.x +
                                  approachVec.y * approachVec.y);
            if (mag > 1e-3f) {
                approachVec.x /= mag;
                approachVec.y /= mag;
            }
        }
        Vector behindPos;
        behindPos.x = d.pos.x + approachVec.x * 2.0f;
        behindPos.y = d.pos.y + approachVec.y * 2.0f;
        behindPos.z = d.pos.z;

        int frontRoom = -1, backRoom = -1;
        acc::engine::GetRoomAtIndexed(area, d.pos,     frontRoom);
        acc::engine::GetRoomAtIndexed(area, behindPos, backRoom);

        float dN = ProbeDistance(d.pos,  0.0f,  1.0f);
        float dE = ProbeDistance(d.pos,  1.0f,  0.0f);
        float dS = ProbeDistance(d.pos,  0.0f, -1.0f);
        float dW = ProbeDistance(d.pos, -1.0f,  0.0f);

        acclog::Write(
            "WallTopo",
            "    door[%d] diag: nearestNode=%d (%.1f,%.1f) approach=(%.2f,%.2f) "
            "front_room=%d back_room=%d (2m past at %.1f,%.1f) | "
            "probes N=%.1fm E=%.1fm S=%.1fm W=%.1fm",
            i, nearestNode,
            nearestNode >= 0 ? g_graph.node_pos[nearestNode].x : 0.0f,
            nearestNode >= 0 ? g_graph.node_pos[nearestNode].y : 0.0f,
            approachVec.x, approachVec.y,
            frontRoom, backRoom, behindPos.x, behindPos.y,
            dN, dE, dS, dW);
    }
}

// Diagnostic: nav-graph vs wall-cache crossing check.
//
// Confirms (and localizes) phantom walls using ONLY engine walkmesh +
// nav data — no .lyt-room topography enters this test. The engine's
// path_points / path_connections are the edges it uses for AI
// pathfinding, so an engine-authored nav edge is provably walkable. If
// such an edge crosses one of our cached "wall" edges, that wall is
// phantom by definition: the engine routes creatures straight through
// it.
//
// The crossing test is 3D-aware: a 2D-projection coincidence where the
// wall sits more than kMaxZcrossM above/below the nav edge (a wall on a
// different floor) is excluded, so this also measures how much of the
// runtime WALL-FILTERED noise is pure 2D-projection (would be fixed by
// a z-overlap guard) versus genuine same-floor phantoms (missed portal
// seams or straight-line-reachability mismatch).
//
// Crossings within kMinNodeClrM of either nav node are ignored — nav
// nodes routinely hug real walls, and a graze at the node endpoint is
// not the engine routing *through* a wall.
//
// Per-wall material_id + room_id are dumped: a phantom carrying a FLOOR
// surfacemat (e.g. Metal/Grass) points at a mis-emitted walkable seam;
// a phantom carrying a wall material points at the straight-line model
// crossing a real wall the path actually skirts. Read-only; never
// mutates the graph or the cache.
void LogNavWallCrossings(const acc::engine::navgraph::NavGraphSnapshot& g) {
    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    if (!acc::spatial::change_detector::GetCachedWalls(walls, wallCount) ||
        !walls || wallCount <= 0) {
        acclog::Write("WallTopo",
                      "  navwall-crosscheck: no wall cache available — skipping");
        return;
    }

    // Shared with the production SegmentCrossesWalkmesh z-guard so the
    // diagnostic measures exactly what the runtime test will reject.
    constexpr float kMaxZcrossM   = acc::engine::kWallCrossZToleranceM;
    constexpr float kMinNodeClrM  = 0.5f;   // crossing must clear both nav nodes
    constexpr float kMinNodeClrSq = kMinNodeClrM * kMinNodeClrM;
    constexpr int   kMaxDump      = 40;

    int n = static_cast<int>(g.nodes.size());
    if (n > kMaxNodes) n = kMaxNodes;

    // Distinct-wall flag so we count phantom WALLS, not phantom crossings
    // (one wall may be crossed by several nav edges). 16 KB static, fine
    // for the single-threaded patch model.
    constexpr int kWallFlagCap = 16384;
    static bool s_crossed[kWallFlagCap];
    int wallN = (wallCount < kWallFlagCap) ? wallCount : kWallFlagCap;
    for (int i = 0; i < wallN; ++i) s_crossed[i] = false;

    int navEdges = 0, crossings = 0, zSkipped = 0, dumped = 0;

    for (int i = 0; i < n; ++i) {
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        for (int e = lo; e < hi; ++e) {
            int j = static_cast<int>(g.conns[e]);
            if (j <= i || j >= n) continue;  // each undirected edge once
            ++navEdges;
            const Vector& P = g.nodes[i].pos;
            const Vector& Q = g.nodes[j].pos;
            float abx = Q.x - P.x, aby = Q.y - P.y;
            if (abx * abx + aby * aby < 1e-6f) continue;

            for (int w = 0; w < wallN; ++w) {
                const acc::engine::WallEdge& we = walls[w];
                float cdx = we.b.x - we.a.x, cdy = we.b.y - we.a.y;
                float denom = abx * cdy - aby * cdx;
                if (denom > -1e-8f && denom < 1e-8f) continue;  // parallel
                float dx = we.a.x - P.x, dy = we.a.y - P.y;
                float t = (dx * cdy - dy * cdx) / denom;  // along nav edge
                float u = (dx * aby - dy * abx) / denom;  // along wall edge
                if (t < 0.0f || t > 1.0f) continue;
                if (u < 0.0f || u > 1.0f) continue;

                float cx = P.x + t * abx, cy = P.y + t * aby;
                float d0x = cx - P.x, d0y = cy - P.y;
                float d1x = cx - Q.x, d1y = cy - Q.y;
                if (d0x * d0x + d0y * d0y < kMinNodeClrSq) continue;
                if (d1x * d1x + d1y * d1y < kMinNodeClrSq) continue;

                // 3D guard: reject different-floor projection coincidences.
                float navZ  = P.z   + t * (Q.z   - P.z);
                float wallZ = we.a.z + u * (we.b.z - we.a.z);
                float adz = navZ - wallZ;
                if (adz < 0.0f) adz = -adz;
                if (adz > kMaxZcrossM) { ++zSkipped; continue; }

                ++crossings;
                s_crossed[w] = true;
                if (dumped < kMaxDump) {
                    ++dumped;
                    acclog::Write(
                        "WallTopo",
                        "  navwall-cross[%d]: nav node[%d](%.1f,%.1f,%.1f) -> "
                        "node[%d](%.1f,%.1f,%.1f) crosses wall[%d] "
                        "(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f) room=%d mat=%d "
                        "at (%.1f,%.1f) dz=%.2f",
                        dumped, i, P.x, P.y, P.z, j, Q.x, Q.y, Q.z,
                        w, we.a.x, we.a.y, we.a.z, we.b.x, we.b.y, we.b.z,
                        we.room_id, we.material_id, cx, cy, adz);
                }
            }
        }
    }

    int distinctWalls = 0;
    for (int w = 0; w < wallN; ++w) if (s_crossed[w]) ++distinctWalls;

    acclog::Write(
        "WallTopo",
        "  navwall-crosscheck: navEdges=%d walls=%d -> crossings=%d "
        "distinct-phantom-walls=%d (z-skipped=%d at |dz|>%.1fm; "
        "node-clearance=%.1fm; dumped=%d, cap=%d)",
        navEdges, wallCount, crossings, distinctWalls, zSkipped,
        kMaxZcrossM, kMinNodeClrM, dumped, kMaxDump);
}

// Diagnostic-only: dump per-node topology metrics so we can pick a
// principled gate later. NO MERGE LOGIC USES THESE COUNTS — pure
// reconnaissance, called from BuildForArea's end-of-build diagnostics
// block alongside the other Log* dumps. For each node we log:
//   - degree      (existing nav-graph degree)
//   - triangles   (3-cycles touching the node — pairs of neighbours that
//                  are directly connected to each other)
//   - 4-cycles    (pairs of neighbours that share a common neighbour ≠ the
//                  node itself, not already counted as a triangle)
//   - 2-hop reach (distinct nodes reachable within 2 graph-hops, ex-self)
// Goal: separate "linear corridor" (triangles=0, 4-cycles=0, low reach)
// from "looped room patrol points" (triangles or 4-cycles ≥1) from
// "T-junction" (high reach, but triangles=0, 4-cycles=0). Run on Endar
// Spire quarters + corridors + bridge and Taris South Apartments to
// compare topology signatures before tuning any gate.
void LogTopologyMetrics(const acc::engine::navgraph::NavGraphSnapshot& g,
                        int n) {
    auto navIsConnected = [&](int a, int b) -> bool {
        if (a < 0 || a >= n || b < 0 || b >= n || a == b) return false;
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, a, lo, hi);
        for (int e = lo; e < hi; ++e) {
            if (static_cast<int>(g.conns[e]) == b) return true;
        }
        return false;
    };
    bool reachVisited[kMaxNodes] = {false};
    acclog::Write(
        "WallTopo",
        "  topology-dump: per-node degree / triangles / 4-cycles / "
        "2-hop reach (diagnostic only — no behaviour change)");
    for (int i = 0; i < n; ++i) {
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        int deg = hi - lo;

        int triangles = 0;
        int fourCycles = 0;
        // For each unordered pair (Y, Z) of i's neighbours.
        for (int a = lo; a < hi; ++a) {
            int Y = static_cast<int>(g.conns[a]);
            if (Y < 0 || Y >= n || Y == i) continue;
            for (int b = a + 1; b < hi; ++b) {
                int Z = static_cast<int>(g.conns[b]);
                if (Z < 0 || Z >= n || Z == i || Z == Y) continue;
                if (navIsConnected(Y, Z)) {
                    ++triangles;
                    continue;  // triangle, not a 4-cycle through this pair
                }
                // 4-cycle: do Y and Z share a common neighbour W ≠ i?
                int yLo = 0, yHi = 0;
                acc::engine::navgraph::NeighbourRange(g, Y, yLo, yHi);
                bool found4 = false;
                for (int c = yLo; c < yHi && !found4; ++c) {
                    int W = static_cast<int>(g.conns[c]);
                    if (W < 0 || W >= n) continue;
                    if (W == i || W == Y || W == Z) continue;
                    if (navIsConnected(W, Z)) found4 = true;
                }
                if (found4) ++fourCycles;
            }
        }

        // 2-hop reach: BFS depth 2 from node i, count distinct.
        for (int k = 0; k < n; ++k) reachVisited[k] = false;
        reachVisited[i] = true;
        int reach = 0;
        for (int a = lo; a < hi; ++a) {
            int Y = static_cast<int>(g.conns[a]);
            if (Y < 0 || Y >= n || reachVisited[Y]) continue;
            reachVisited[Y] = true;
            ++reach;
            int yLo = 0, yHi = 0;
            acc::engine::navgraph::NeighbourRange(g, Y, yLo, yHi);
            for (int b = yLo; b < yHi; ++b) {
                int Z = static_cast<int>(g.conns[b]);
                if (Z < 0 || Z >= n || reachVisited[Z]) continue;
                reachVisited[Z] = true;
                ++reach;
            }
        }

        acclog::Write(
            "WallTopo",
            "    node[%d] (%.1f,%.1f) deg=%d tri=%d c4=%d reach2=%d",
            i, g.nodes[i].pos.x, g.nodes[i].pos.y,
            deg, triangles, fourCycles, reach);
    }
}

// Diagnostic-only: per multi-node cluster, dump member adjacency. For
// each member node, list its graph neighbours split into internal (same
// cluster) and external (cross-cluster), and mark the edge's ClassifyEdge
// verdict. Passes nullptr for the area diag so this re-walk doesn't
// double-emit caveat-2 lines already logged during collection. Reads the
// frozen union-find state, so call it after all merge passes complete.
void LogClusterMemberAdjacency(const acc::engine::navgraph::NavGraphSnapshot& g,
                               int n) {
    for (int root = 0; root < n; ++root) {
        if (UFFind(root) != root) continue;
        int size = 0;
        for (int m = 0; m < n; ++m) if (UFFind(m) == root) ++size;
        if (size < 2) continue;
        acclog::Write("WallTopo",
                      "  cluster[%d] size=%d label=\"%s\"",
                      root, size, g_graph.node_label[root].c_str());
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            int lo = 0, hi = 0;
            acc::engine::navgraph::NeighbourRange(g, m, lo, hi);
            for (int e = lo; e < hi; ++e) {
                int nb = static_cast<int>(g.conns[e]);
                if (nb < 0 || nb >= n) continue;
                bool internal = (UFFind(nb) == root);
                EdgeResult er = ClassifyEdge(/*areaForDiag=*/nullptr,
                                             g.nodes[m].pos,
                                             g.nodes[nb].pos,
                                             "dump");
                if (er.kind == kEdgeDoor) {
                    acclog::Write(
                        "WallTopo",
                        "    member[%d] (%.1f,%.1f) -> nb[%d] (%.1f,%.1f) "
                        "%s door[%d] transition=\"%s\"",
                        m, g.nodes[m].pos.x, g.nodes[m].pos.y,
                        nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y,
                        internal ? "INTERNAL" : "EXTERNAL",
                        er.doorIdx,
                        g_graph.doors[er.doorIdx].transitionDest[0]
                            ? g_graph.doors[er.doorIdx].transitionDest
                            : "(none)");
                } else if (er.kind == kEdgeBlocked) {
                    acclog::Write(
                        "WallTopo",
                        "    member[%d] (%.1f,%.1f) -> nb[%d] (%.1f,%.1f) "
                        "%s WALL-BLOCKED",
                        m, g.nodes[m].pos.x, g.nodes[m].pos.y,
                        nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y,
                        internal ? "INTERNAL" : "EXTERNAL");
                } else {
                    acclog::Write(
                        "WallTopo",
                        "    member[%d] (%.1f,%.1f) -> nb[%d] (%.1f,%.1f) "
                        "%s clear",
                        m, g.nodes[m].pos.x, g.nodes[m].pos.y,
                        nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y,
                        internal ? "INTERNAL" : "EXTERNAL");
                }
            }
        }
    }
}

}  // namespace acc::room_topology
