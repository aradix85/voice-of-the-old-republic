// Pass 3 of the build: gives every cluster its kind (dead end, corridor,
// junction, room, area) and label, from its members' shape flags, its
// footprint and its external exits, and writes the result to every member.

#include "room_topology_internal.h"

#include "log.h"

namespace acc::room_topology {

namespace {

// Zone scale: a node spread past this is a place, not a point. Between the
// biggest real crossing (under 10 m) and the smallest zone-web seen (46 m);
// the node box under-reads the true footprint.
constexpr float kJunctionZoneExtentMeters = 15.0f;

// For one or two exits, zone-scale length alone is a corridor: a place must
// be this wide too. Vanilla hallways run 2.5-3 m; the Ebon Hawk's hull
// fields are 7.1 and 10.6 m.
constexpr float kZoneMinWidthM = 6.0f;

// A cluster's members measured together: centroid, shape flags, mean
// calibrated floor z, and the member-node bounding box.
struct ClusterGeom {
    Vector centroid = {0.0f, 0.0f, 0.0f};
    int    size     = 0;
    bool   hasOpen  = false;
    bool   hasRoom  = false;
    float  floorZ   = 0.0f;
    float  minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    // Longer and shorter bounding-box side. Node positions, not walls, so
    // it under-reads the true area, but a sprawling open cluster (~160 m)
    // and a normal merged room (~20-30 m) are far apart. Single-node
    // clusters have zero extent and can never be "large"; the shorter side
    // feeds the zone-scale rule (a run is long AND narrow).
    float  extent   = 0.0f;
    float  minSide  = 0.0f;
};

ClusterGeom MeasureCluster(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root,
                           const BuildPasses& bp) {
    ClusterGeom c;
    float floorZSum = 0.0f;
    for (int m = 0; m < n; ++m) {
        if (UFFind(m) != root) continue;
        const Vector& p = g.nodes[m].pos;
        c.centroid.x += p.x;
        c.centroid.y += p.y;
        c.centroid.z += p.z;
        if (p.x < c.minX) c.minX = p.x;
        if (p.x > c.maxX) c.maxX = p.x;
        if (p.y < c.minY) c.minY = p.y;
        if (p.y > c.maxY) c.maxY = p.y;
        if (bp.nodeOpen[m]) c.hasOpen = true;
        if (bp.nodeRoom[m]) c.hasRoom = true;
        floorZSum += bp.nodeFloorZ[m];
        ++c.size;
    }
    if (c.size > 0) {
        c.centroid.x /= c.size;
        c.centroid.y /= c.size;
        c.centroid.z /= c.size;
        c.floorZ = floorZSum / c.size;
        const float w = c.maxX - c.minX;
        const float h = c.maxY - c.minY;
        c.extent  = (w > h) ? w : h;
        c.minSide = (w > h) ? h : w;
    }
    return c;
}

// Per-axis extent of the cluster = median over its members' own 8-ray
// extents. Feeds the elongation verdict (ComputeCentroidAxis explains why
// one centroid probe is not enough). Median, not mean: a member sitting on
// a crossing corridor reads tens of metres along that corridor and must not
// drag the verdict. False without per-node data.
bool MedianAxisExtents(int n, int root, const BuildPasses& bp, float axisExt[4]) {
    if (!bp.haveGW) return false;
    bool have = false;
    float extTmp[kMaxNodes];
    for (int a = 0; a < 4; ++a) {
        int cnt = 0;
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            extTmp[cnt++] = bp.nodeAxisExt[m][a];
        }
        if (cnt <= 0) break;
        std::nth_element(extTmp, extTmp + cnt / 2, extTmp + cnt);
        axisExt[a] = extTmp[cnt / 2];
        have = true;
    }
    return have;
}

// External neighbours: edges from any member to a non-member, deduped by
// node id. The source member is kept with each so ClassifyCluster's
// door-on-edge test uses the ACTUAL graph edge (member -> nb) rather than
// the centroid -> nb blur: a plaza centroid is a synthesized midpoint, and
// testing from it matched doors on no real edge (the Upper-City plaza lit
// up "Door North, Door East, Door South, Door West" at once). Each edge
// runs through ClassifyEdge: a blocked edge drops (the walkmesh says it
// crosses a wall); door verdicts are cached so ClassifyCluster needs no
// second door query.
constexpr int kMaxExternal = 16;

struct ClusterExits {
    int nbs[kMaxExternal];
    int srcs[kMaxExternal];     // the cluster member that owns the edge
    int doorIdx[kMaxExternal];
    int count = 0;
};

void CollectExternalExits(void* area, const acc::engine::navgraph::NavGraphSnapshot& g, int n,
                          int root, ClusterExits& ex) {
    for (int m = 0; m < n; ++m) {
        if (UFFind(m) != root) continue;
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, m, lo, hi);
        for (int e = lo; e < hi; ++e) {
            const int nb = static_cast<int>(g.conns[e]);
            if (nb < 0 || nb >= n) continue;
            if (UFFind(nb) == root) continue;  // internal edge
            const EdgeResult er = ClassifyEdge(area, g.nodes[m].pos, g.nodes[nb].pos, "ext-edge");
            if (er.kind == kEdgeBlocked) {
                acclog::Write(
                    "WallTopo",
                    "  external BLOCKED: cluster=%d member[%d] (%.1f,%.1f) "
                    "-> nb[%d] (%.1f,%.1f) — segment crosses wall, "
                    "dropping from neighbour list",
                    root, m, g.nodes[m].pos.x, g.nodes[m].pos.y,
                    nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y);
                continue;
            }
            bool seen = false;
            for (int k = 0; k < ex.count; ++k) {
                if (ex.nbs[k] != nb) continue;
                // A later edge to the same neighbour may find the door the
                // first one missed.
                if (ex.doorIdx[k] < 0 && er.kind == kEdgeDoor) ex.doorIdx[k] = er.doorIdx;
                seen = true;
                break;
            }
            if (seen) continue;
            if (ex.count < kMaxExternal) {
                ex.nbs[ex.count]     = nb;
                ex.srcs[ex.count]    = m;
                ex.doorIdx[ex.count] = (er.kind == kEdgeDoor) ? er.doorIdx : -1;
                ++ex.count;
            } else {
                // This exit will not be voiced: logged so an under-reported
                // plaza shows in the harvest (raise kMaxExternal if real).
                acclog::Write(
                    "WallTopo",
                    "  external CAP: cluster=%d already has "
                    "kMaxExternal=%d neighbours — dropping exit to "
                    "nb[%d] (%.1f,%.1f); this exit will be missing "
                    "from the label",
                    root, kMaxExternal, nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y);
            }
        }
    }
}

// How the cluster announces: 0 = by its exit topology (junction, corridor,
// dead end), 1 = room, 2 = big area. A cluster is an area only when the
// clearance probe says it is a PLACE: an open-class member, a multi-node
// room cluster, or an isolated cluster with no exits. A merged hub with no
// open or room geometry stays a junction, a degree-2 run a corridor.
//
// The zone-scale rule then makes any cluster whose member footprint is
// zone-scale a place, as the crossing / run / cul-de-sac words promise
// something a few steps wide: with 3+ exits whatever its shape (Lehon
// Southern Strand, 46 m, re-announced as a new crossing from every fringe
// channel); with 1-2 exits only when wide too, since a real corridor is
// long and narrow (the Ebon-Hawk port deck, a 34-node field with one ramp
// out, read as a dead end).
int AreaHint(const ClusterGeom& c, int externalCount, int root) {
    int hint = 0;
    if (externalCount == 0 || c.hasOpen || (c.size > 1 && c.hasRoom)) {
        hint = (c.hasRoom && !c.hasOpen) ? 1 : 2;
    }
    if (hint == 0 && c.extent >= kJunctionZoneExtentMeters &&
        (externalCount >= 3 || c.minSide >= kZoneMinWidthM)) {
        hint = 2;
        acclog::Write("WallTopo",
                      "  zone-scale: cluster=%d size=%d extent=%.1fm "
                      "minside=%.1fm exits=%d (>= %.1fm, and 3+ exits "
                      "or >= %.1fm wide) — rendered as an area",
                      root, c.size, c.extent, c.minSide,
                      externalCount, kJunctionZoneExtentMeters,
                      kZoneMinWidthM);
    }
    return hint;
}

// Transition triggers whose nearest nav node is a member become named exits.
constexpr int kMaxClusterTransitions = 4;

int CollectTransitions(int n, int root, int trigIdxs[kMaxClusterTransitions]) {
    int count = 0;
    for (int t = 0; t < g_graph.transition_count && count < kMaxClusterTransitions; ++t) {
        const int nn = g_graph.transitions[t].nearestNode;
        if (nn >= 0 && nn < n && UFFind(nn) == root) trigIdxs[count++] = t;
    }
    return count;
}

// The dead-end verdict for the cluster, from its degree-1 members (where a
// terminal wall would be), not the centroid, which for a multi-node run
// sits mid-corridor. One confirming terminal is enough; a cluster with no
// degree-1 member cannot be judged and is not filtered.
bool TerminalDeadEndAgrees(const acc::engine::navgraph::NavGraphSnapshot& g, int n, int root,
                           const BuildPasses& bp) {
    bool sawTerminal = false;
    for (int m = 0; m < n; ++m) {
        if (UFFind(m) != root || Degree(g, m) != 1) continue;
        if (bp.nodeDeadEnd[m]) return true;
        sawTerminal = true;
    }
    return !sawTerminal;
}

// The floor footprint for announce timing (ClusterInfo::footprintM). Node
// spread reads a single-node crossing as zero metres wide; the narrowest
// wall-to-wall axis through the members is its real width (a Taris
// apartment-hall T junction: about 11 m, node spread 0).
float Footprint(float extent, const float* axisExt) {
    float footprint = extent;
    if (axisExt) {
        float narrowest = axisExt[0];
        for (int a = 1; a < 4; ++a) {
            if (axisExt[a] < narrowest) narrowest = axisExt[a];
        }
        if (narrowest > footprint) footprint = narrowest;
    }
    return footprint;
}

void Tally(int kind, BuildPasses& bp) {
    switch (kind) {
        case kKindDeadEnd:  ++bp.deadEnds;  break;
        case kKindCorridor: ++bp.corridors; break;
        case kKindJunction:
        case kKindPlaza:
        case kKindRoom:     ++bp.junctions; break;  // plazas and rooms tally as junctions
        default:            ++bp.openAreas; break;
    }
}

}  // namespace

// For each cluster root (the smallest node id in its union-find class):
// measure it, collect its external exits, classify it via ClassifyCluster,
// and write the result to every member.
void PassClassifyClusters(void* area,
                          const acc::engine::navgraph::NavGraphSnapshot& g,
                          int n, BuildPasses& bp) {
    bp.clusters = bp.multiNodeClusters = 0;
    bp.deadEnds = bp.corridors = bp.junctions = bp.openAreas = 0;
    for (int root = 0; root < n; ++root) {
        if (UFFind(root) != root) continue;
        ++bp.clusters;

        const ClusterGeom c = MeasureCluster(g, n, root, bp);
        if (c.size > 1) ++bp.multiNodeClusters;
        const bool isLargeArea = c.extent >= kLargeAreaExtentMeters;
        float axisExt[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        const bool haveAxisExt = c.size > 0 && MedianAxisExtents(n, root, bp, axisExt);

        ClusterExits ex;
        CollectExternalExits(area, g, n, root, ex);
        const int areaHint = AreaHint(c, ex.count, root);
        int trigIdxs[kMaxClusterTransitions];
        const int trigCount = CollectTransitions(n, root, trigIdxs);

        std::string label;
        int kind = kKindOpenArea, sig = 0;
        bool filtered = false;
        float bboxXY[4] = {c.minX, c.minY, c.maxX, c.maxY};
        std::vector<ExitMark> exits;
        g_exit_sink = &exits;
        ClassifyCluster(g, c.centroid, ex.nbs, ex.srcs, ex.doorIdx, ex.count,
                        trigIdxs, trigCount,
                        areaHint, c.floorZ, isLargeArea,
                        (c.size > 0) ? bboxXY : nullptr,
                        haveAxisExt ? axisExt : nullptr,
                        bp.nodeDeadEnd, TerminalDeadEndAgrees(g, n, root, bp),
                        label, kind, sig, filtered);
        g_exit_sink = nullptr;
        Tally(kind, bp);

        if (isLargeArea) {
            acclog::Write("WallTopo",
                          "  large-area: cluster=%d size=%d extent=%.1fm "
                          "(>= %.1fm) -> '%s'",
                          root, c.size, c.extent, kLargeAreaExtentMeters,
                          label.c_str());
        }

        const float footprint = Footprint(c.extent, haveAxisExt ? axisExt : nullptr);
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            g_graph.node_label[m]     = label;
            g_graph.node_exits[m]     = exits;
            g_graph.node_kind[m]      = kind;
            g_graph.node_sig[m]       = sig;
            g_graph.node_filtered[m]  = filtered;
            g_graph.node_extent[m]    = c.extent;
            g_graph.node_centre[m]    = c.centroid;
            g_graph.node_footprint[m] = footprint;
        }
    }
}

}  // namespace acc::room_topology
