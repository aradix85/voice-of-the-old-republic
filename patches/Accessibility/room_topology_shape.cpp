// The shape of nodes and regions: per-node features from wall probes, the
// centroid axis of a region, and the pass that gives every region its kind
// (dead end, corridor, junction, room, open area).

#include "room_topology_internal.h"

#include "log.h"
#include "spatial_wall_surfaces.h"  // GetCachedWalls — seam-filtered perimeter cache

namespace acc::room_topology {

namespace {

// Shape-diagnostic thresholds (the per-node class log, NOT behaviour).
// Reconnaissance for the planned generalized shape-classifier: the open
// band uses 8m/6-rays; rooms and corridors sit at a smaller scale, so
// they get their own measured cuts here. Pure logging — only nodeOpen
// (above) drives merges. Tune from the harvested class distribution.
//   kStructRayDistM   — "arm/exit" scale for corridor (2 opposite) and
//                       junction (3+) detection. Corridor nodes between
//                       close junctions don't reach 8m, so 5m is the cut.
//   kRoomNearM        — below this a ray is "wall-hugging", not openness.
//   kRoomBoundedMaxM  — a room must be enclosed within this (no long arm).
//   kRoomMinMidRays   — a real room is evenly open: this many rays must
//                       land in the [near, open) mid-band. Separates a
//                       room interior from a corner/doorway node (mostly
//                       sub-near rays + one escape).
constexpr float kStructRayDistM   = 5.0f;
constexpr float kRoomNearM        = 2.0f;
constexpr float kRoomBoundedMaxM  = 10.0f;
constexpr int   kRoomMinMidRays   = 3;

// Room-class probe. A node is "room" when it sits in a bounded, evenly-open
// space: 0-1 rays clear the open scale, max ≤ kRoomBoundedMaxM, and
// ≥ kRoomMinMidRays rays land in the [near, open) mid-band (the evenness
// test that separates a real room interior from a wall-hugging corner /
// doorway node). This flag, like nodeOpen, no longer drives a merge rule of
// its own — it marks the node as SPACE for the unified Pass 1 space rule.
//
// Measured: it fires where small rooms are (Ebon Hawk 21, Apartments 9) and
// not outdoors (Slums 3).
// Area geometry cue: the only shape we speak for a "Area" is its long
// axis, and only when the space is clearly elongated. At the cluster
// centroid the full extent across each of the 4 axes is the sum of its
// two opposite rays; the space is "elongated" when the longest axis
// exceeds kAxisElongRatio × the perpendicular one. Square / round / open
// spaces fall below the ratio and get no axis word (just "Area").
constexpr float kAxisElongRatio = 1.8f;

// Zone-scale hub threshold: a merged junction web (3+ external exits,
// no open/room member) whose member-node bounding box exceeds this
// renders through the area path ("Area. exits: ...") instead of
// "junction". The word "junction" promises a decision point a few steps
// wide; the Lehon Southern Strand rock-web cluster spans 46m and its
// re-announcement on every re-entry from a fringe channel read as
// discovering a new crossing each time — four phantom crossings in 80s.
// Step-scale merged hubs (double junctions, ~5m) stay
// junction. Sized between the biggest legitimate crossing (<10m) and
// the smallest observed zone-web (46m); node-bbox under-reads true
// footprint, so 15m of node spread is already a place, not a point.
constexpr float kJunctionZoneExtentMeters = 15.0f;

// Width half of the zone-scale rule (Ebon-Hawk hull). For
// clusters with fewer than three exits, zone-scale LENGTH alone isn't
// enough to call a space a place — a corridor is long by nature. The
// min-side test is the discriminator: vanilla hallways run 2.5-3m wide,
// so anything sprawling past this in its SHORT dimension is a field,
// not a run. Sized well above corridor width and below the observed
// fields (hull port deck 10.6m, comb 7.1m) so both read as places
// while hallways keep their corridor voice.
constexpr float kZoneMinWidthM = 6.0f;

}  // namespace

// Cluster classification (degree-driven, with junction-merge support).

// Per-node degree from the snapshot. Tiny helper — every classifier
// path needs this and it keeps the call sites self-documenting.
int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node) {
    int lo = 0, hi = 0;
    acc::engine::navgraph::NeighbourRange(g, node, lo, hi);
    return hi - lo;
}

// Per-node shape features from the 8-ray clearance probe. Fills the
// openness + room flags and the calibrated floor height that the
// open-coalesce and room-coalesce passes consume, and logs the per-node
// clearance/shape profile for ongoing harvest. Runs once at the top of
// BuildForArea, before any merge pass, so every pass sees the same flags.
//
// nodes are stored at z=0 (logically 2D) but walls sit at the real floor
// height; a z=0 ray runs under every wall and the 2m z-guard rejects
// them all, so each node's probe is lifted to the z of its nearest wall
// endpoint. See the shape-band constant block for the thresholds; the
// raw rays + every feature are logged so thresholds stay re-pickable.
void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g,
                              int n, bool* nodeOpen, bool* nodeRoom,
                              float* nodeFloorZ, bool* nodeDeadEnd,
                              float (*nodeAxisExt)[4]) {
    for (int i = 0; i < n; ++i) {
        nodeOpen[i] = false; nodeRoom[i] = false; nodeFloorZ[i] = 0.0f;
        nodeDeadEnd[i] = false;
        for (int a = 0; a < 4; ++a) nodeAxisExt[i][a] = 0.0f;
    }
    const acc::engine::WallEdge* cw = nullptr;
    int cwCount = 0;
    if (!acc::spatial::wall_surfaces::GetCachedWalls(cw, cwCount) ||
        !cw || cwCount <= 0) {
        acclog::Write("WallTopo",
                      "  clearance-dump: no wall cache — open/room coalesce "
                      "skipped (no shape data)");
        return;
    }
    acclog::Write("WallTopo",
                  "  clearance-dump: per-node 8-ray shape "
                  "(feeds open + room coalesce)");
    for (int i = 0; i < n; ++i) {
        Vector probePos = g.nodes[i].pos;
        // Height-band vote (wall_probe::ResolveProbeZ). Replaced the
        // single-nearest-endpoint lift that lived here: when a door
        // opening dropped the nearest wall from the cache, the lift
        // latched onto a multi-storey anomaly edge 4m overhead and the
        // all-clear probe reclassified a dead end pocket as open space
        // (mine shafts node 76).
        probePos.z = acc::wall_probe::ResolveProbeZ(cw, cwCount, probePos);
        nodeFloorZ[i] = probePos.z;
        float r[8];
        ProbeClearance8(cw, cwCount, probePos, r);
        // Full extent across each axis (opposite rays summed), kept per
        // node so Pass 3 can judge a cluster's elongation from ALL its
        // members instead of one probe at the centroid — see
        // ComputeCentroidAxis for why the centroid alone misleads.
        // Index 0=E-W, 1=NE-SW, 2=N-S, 3=NW-SE.
        for (int a = 0; a < 4; ++a) nodeAxisExt[i][a] = r[a] + r[a + 4];
        float mn = r[0], mx = r[0];
        int mnBit = 0, mxBit = 0;
        for (int k = 1; k < 8; ++k) {
            if (r[k] < mn) { mn = r[k]; mnBit = k; }
            if (r[k] > mx) { mx = r[k]; mxBit = k; }
        }
        int openRays = 0, structN = 0, midN = 0, shortN = 0;
        int longBits = 0, structBits = 0;
        float sum = 0.0f;
        for (int k = 0; k < 8; ++k) {
            sum += r[k];
            if (r[k] > kOpenRayDistM)   { ++openRays; longBits   |= (1 << k); }
            if (r[k] > kStructRayDistM) { ++structN;  structBits |= (1 << k); }
            if (r[k] >= kRoomNearM && r[k] < kOpenRayDistM) ++midN;
            if (r[k] < kRoomNearM) ++shortN;
        }
        float mean = sum / 8.0f;
        nodeOpen[i] = (openRays >= kOpenMinRays);
        nodeRoom[i] = (openRays <= 1) && (mx <= kRoomBoundedMaxM) &&
                      (midN >= kRoomMinMidRays);

        // Tentative shape class — diagnostic label only (see the shape-band
        // constants). Connector bands count genuine long arms (>8m); rooms
        // are caught by mid-band evenness, not arm count.
        const char* shape;
        if (openRays >= kOpenMinRays) {
            shape = "open";
        } else if (openRays >= 3) {
            shape = "junction";
        } else if (openRays == 2) {
            int b0 = -1, b1 = -1;
            for (int k = 0; k < 8; ++k) {
                if (longBits & (1 << k)) { if (b0 < 0) b0 = k; else b1 = k; }
            }
            int d = (b0 >= 0 && b1 >= 0) ? (b1 - b0) : 0;
            if (d > 4) d = 8 - d;
            shape = (d >= 3) ? "corridor" : "corner";
        } else if (openRays == 1) {
            shape = nodeRoom[i] ? "room" : "deadend";
        } else {
            shape = nodeRoom[i] ? "room" : "corner";
        }

        acclog::Write(
            "WallTopo",
            "    clearance node[%d] (%.1f,%.1f) deg=%d probeZ=%.1f "
            "min=%.1f@%d max=%.1f@%d mean=%.1f openN=%d structN=%d "
            "midN=%d shortN=%d open=%d room=%d structbits=0x%02x class=%s "
            "rays[E,NE,N,NW,W,SW,S,SE]="
            "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            i, g.nodes[i].pos.x, g.nodes[i].pos.y, Degree(g, i), probePos.z,
            mn, mnBit, mx, mxBit, mean, openRays, structN,
            midN, shortN, nodeOpen[i] ? 1 : 0, nodeRoom[i] ? 1 : 0,
            structBits, shape,
            r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
    }

    // Per-node dead-end verdict. "Does the walkmesh agree
    // that this degree-1 node terminates a region?" — asked once, here,
    // with the other node-local shape facts, instead of four times in
    // the classification pass from three different probe origins.
    //
    // Why the question exists at all: the nav graph is hand-authored by
    // BioWare's level designers for AI patrol routing. They drop
    // degree-1 nodes at wall bumps, room corners and patrol terminators,
    // many of which are nothing a player can walk into. The graph can't
    // tell those from genuine recesses — both are degree-1 — but the
    // walkmesh can: a real alcove has walls on three sides and an
    // entrance on the fourth, while an artefact looks like corridor or
    // open space at the node's position. The probe runs along the actual
    // graph-edge axis rather than the compass, because a diagonally
    // aligned alcove reads as a junction under cardinal rays. Missing
    // wall data fails OPEN (trust the graph rather than over-filter).
    //
    // Why it moved: the verdict is a TOPOLOGY fact, not just a naming
    // guard. Computed in Pass 3 it arrived after every merge had run,
    // so a rejected dead-end could not be un-clustered — that is what
    // the "filtered / rescue-only" flag was papering over, and what let
    // three interconnected hull lanes stand as silent ghost dead ends
    // (Ebon-Hawk 002ebo). Computed here, the merge passes can consume it.
    //
    // The probe runs from the NODE along its single graph edge. The old
    // ClassifyAsDeadEnd call site passed the CLUSTER CENTROID instead,
    // which for a multi-node lane sits mid-run with metres of corridor
    // behind it — the "wall close behind" test could never pass, so
    // every dead-end run longer than a few metres was filtered. Asking
    // per node removes that whole class of false negative.
    for (int i = 0; i < n; ++i) {
        if (Degree(g, i) != 1) continue;
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        if (lo >= hi) continue;
        int nb = static_cast<int>(g.conns[lo]);
        if (nb < 0 || nb >= n) continue;
        Vector p = g.nodes[i].pos;
        p.z = nodeFloorZ[i];
        nodeDeadEnd[i] = IsAlcoveAlongAxis(p,
                                           g.nodes[nb].pos.x - p.x,
                                           g.nodes[nb].pos.y - p.y);
        if (!nodeDeadEnd[i]) {
            acclog::Write("WallTopo",
                          "    deadend-verdict node[%d] (%.1f,%.1f) -> "
                          "NOT a real dead end (walkmesh continues past it)",
                          i, g.nodes[i].pos.x, g.nodes[i].pos.y);
        }
    }
}

// Long-axis test for an area cluster: is the space clearly elongated,
// and along which axis word? Returns false (no axis) for roughly
// square / round / open spaces — those speak as a bare "Area". The
// axis is the only geometry cue we surface; there is no size.
//
// memberExt: the cluster's per-axis extents (index 0=E-W, 1=NE-SW,
// 2=N-S, 3=NW-SE), each the MEDIAN over the member nodes' own 8-ray
// probes (ComputeNodeShapeFeatures). Preferred over a single probe at
// the centroid, which is what this used to be: the centroid of a
// corridor sits mid-run, and mid-run is exactly where a crossing
// corridor opens up. Citadel Station Wohnmodul 082 East,
// the Apartmentkomplex-C corridor: 11 nodes spanning 23m east-west,
// 9 of them boxed to ~9m north-south, but the centroid landed on the
// entrance corridor crossing it and its N-S rays ran 50m through the
// door — "Area North-South" for a corridor you walk east-west. The
// median ignores the two crossing members. nullptr falls back to the
// centroid probe (no per-node data — wall cache missing at feature
// time, in which case the probe here fails the same way).
bool ComputeCentroidAxis(const Vector& centroid, float floorZ,
                         const float* memberExt,
                         acc::strings::Id& outAxisId) {
    using acc::strings::Id;
    float ext[4];
    if (memberExt) {
        for (int a = 0; a < 4; ++a) ext[a] = memberExt[a];
    } else {
        const acc::engine::WallEdge* cw = nullptr;
        int cwCount = 0;
        if (!acc::spatial::wall_surfaces::GetCachedWalls(cw, cwCount) ||
            !cw || cwCount <= 0) {
            return false;
        }
        Vector p = centroid;
        p.z = floorZ;
        float r[8];
        ProbeClearance8(cw, cwCount, p, r);
        // Full extent across each of the 4 axes = sum of the opposite
        // rays (octant pairs k and k+4).
        for (int a = 0; a < 4; ++a) ext[a] = r[a] + r[a + 4];
    }
    int longA = 0;
    for (int a = 1; a < 4; ++a) if (ext[a] > ext[longA]) longA = a;
    int perpA = (longA + 2) % 4;  // 90° away
    // ext[longA] > 0 guards the all-zero case (per-node data never
    // computed) — 0 >= 1.8 * 0 would otherwise claim an "East-West" axis.
    bool elong = ext[longA] > 0.0f &&
                 ext[longA] >= kAxisElongRatio * ext[perpA];
    acclog::Write("WallTopo",
                  "ComputeCentroidAxis: at (%.1f,%.1f) %s extents "
                  "E-W=%.1f NE-SW=%.1f N-S=%.1f NW-SE=%.1f -> %s",
                  centroid.x, centroid.y,
                  memberExt ? "member-median" : "centroid-probe",
                  ext[0], ext[1], ext[2], ext[3],
                  elong ? (longA == 0 ? "E-W" : longA == 1 ? "NE-SW"
                           : longA == 2 ? "N-S" : "NW-SE")
                        : "no axis");
    if (!elong) return false;
    switch (longA) {
        case 0: outAxisId = Id::AxisEastWest;   break;
        case 2: outAxisId = Id::AxisNorthSouth; break;
        case 1: outAxisId = BitToOctant(1);     break;  // NE-SW → "North-East"
        case 3: outAxisId = BitToOctant(3);     break;  // NW-SE → "North-West"
    }
    return true;
}

// The two opposite octant bits an elongation axis covers, or 0 for a
// non-axis Id. Used to drop a redundant exit list: an "East-West" area whose
// only exits are exactly East + West has an exit list that just echoes the
// axis word, so we speak "Area East-West" alone. The four values mirror
// the axisId cases ComputeCentroidAxis can emit.
int AxisOctantMask(acc::strings::Id axisId) {
    using acc::strings::Id;
    if (axisId == Id::AxisEastWest)   return (1 << 0) | (1 << 4);  // E + W
    if (axisId == Id::AxisNorthSouth) return (1 << 2) | (1 << 6);  // N + S
    if (axisId == Id::DirNortheast)   return (1 << 1) | (1 << 5);  // NE + SW
    if (axisId == Id::DirNorthwest)   return (1 << 3) | (1 << 7);  // NW + SE
    return 0;
}

// ===== Pass 3: per-cluster classification =====

namespace {

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
                      "or >= %.1fm wide) — rendered as Bereich",
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
