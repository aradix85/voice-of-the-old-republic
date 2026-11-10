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
// Area geometry cue: the only shape we speak for a "Bereich" is its long
// axis, and only when the space is clearly elongated. At the cluster
// centroid the full extent across each of the 4 axes is the sum of its
// two opposite rays; the space is "elongated" when the longest axis
// exceeds kAxisElongRatio × the perpendicular one. Square / round / open
// spaces fall below the ratio and get no axis word (just "Bereich").
constexpr float kAxisElongRatio = 1.8f;

// Zone-scale hub threshold: a merged junction web (3+ external exits,
// no open/room member) whose member-node bounding box exceeds this
// renders through the area path ("Bereich. Ausgänge: ...") instead of
// "Kreuzung". The word "Kreuzung" promises a decision point a few steps
// wide; the Lehon Südlicher Strand rock-web cluster spans 46m and its
// re-announcement on every re-entry from a fringe channel read as
// discovering a new crossing each time — four phantom crossings in 80s.
// Step-scale merged hubs (double junctions, ~5m) stay
// Kreuzung. Sized between the biggest legitimate crossing (<10m) and
// the smallest observed zone-web (46m); node-bbox under-reads true
// footprint, so 15m of node spread is already a place, not a point.
constexpr float kJunctionZoneExtentMeters = 15.0f;

// Width half of the zone-scale rule (Ebon-Hawk hull). For
// clusters with fewer than three exits, zone-scale LENGTH alone isn't
// enough to call a space a place — a corridor is long by nature. The
// min-side test is the discriminator: vanilla hallways run 2.5-3m wide,
// so anything sprawling past this in its SHORT dimension is a field,
// not a run. Sized well above corridor width and below the observed
// fields (hull Backbord deck 10.6m, comb 7.1m) so both read as places
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
        // all-clear probe reclassified a Sackgasse pocket as open space
        // (Minenschächte node 76).
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
    // three interconnected hull lanes stand as silent ghost Sackgassen
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
// square / round / open spaces — those speak as a bare "Bereich". The
// axis is the only geometry cue we surface; there is no size.
//
// memberExt: the cluster's per-axis extents (index 0=E-W, 1=NE-SW,
// 2=N-S, 3=NW-SE), each the MEDIAN over the member nodes' own 8-ray
// probes (ComputeNodeShapeFeatures). Preferred over a single probe at
// the centroid, which is what this used to be: the centroid of a
// corridor sits mid-run, and mid-run is exactly where a crossing
// corridor opens up. Citadel Station Wohnmodul 082 Ost,
// the Apartmentkomplex-C corridor: 11 nodes spanning 23m east-west,
// 9 of them boxed to ~9m north-south, but the centroid landed on the
// entrance corridor crossing it and its N-S rays ran 50m through the
// door — "Bereich Nord-Süd" for a corridor you walk east-west. The
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
    // computed) — 0 >= 1.8 * 0 would otherwise claim an "Ost-West" axis.
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
        case 1: outAxisId = BitToOctant(1);     break;  // NE-SW → "Nord-Ost"
        case 3: outAxisId = BitToOctant(3);     break;  // NW-SE → "Nord-West"
    }
    return true;
}

// The two opposite octant bits an elongation axis covers, or 0 for a
// non-axis Id. Used to drop a redundant exit list: an "Ost-West" area whose
// only exits are exactly Ost + West has an exit list that just echoes the
// axis word, so we speak "Bereich Ost-West" alone. The four values mirror
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
// For each cluster root (the
// smallest node id in its union-find class), compute the centroid,
// collect external neighbours (dedup by node id), classify via
// ClassifyCluster, and write the result to every member.
void PassClassifyClusters(void* area,
                          const acc::engine::navgraph::NavGraphSnapshot& g,
                          int n, BuildPasses& bp) {
    const bool*  nodeOpen   = bp.nodeOpen;
    const bool*  nodeRoom   = bp.nodeRoom;
    const float* nodeFloorZ = bp.nodeFloorZ;

    int clusters = 0, multiNodeClusters = 0;
    int deadEnds = 0, corridors = 0, junctions = 0, openAreas = 0;
    for (int root = 0; root < n; ++root) {
        if (UFFind(root) != root) continue;
        ++clusters;

        // Centroid of all members. Also tally member shape flags + the
        // calibrated floor z (averaged) so we can decide whether this
        // cluster is a probe-owned area and feed the axis probe.
        Vector centroid = {0.0f, 0.0f, 0.0f};
        int size = 0;
        bool hasOpen = false, hasRoom = false;
        float floorZSum = 0.0f;
        // Member-node bounding box (X/Y) → footprint extent for the
        // large-area label swap. Node positions, not walls, so it under-
        // reads true area, but the relative gap between a sprawling open
        // cluster (~160 m) and a normal merged room (~20-30 m) is huge,
        // so a coarse threshold separates them cleanly.
        float minX =  1e30f, minY =  1e30f;
        float maxX = -1e30f, maxY = -1e30f;
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            centroid.x += g.nodes[m].pos.x;
            centroid.y += g.nodes[m].pos.y;
            centroid.z += g.nodes[m].pos.z;
            if (g.nodes[m].pos.x < minX) minX = g.nodes[m].pos.x;
            if (g.nodes[m].pos.x > maxX) maxX = g.nodes[m].pos.x;
            if (g.nodes[m].pos.y < minY) minY = g.nodes[m].pos.y;
            if (g.nodes[m].pos.y > maxY) maxY = g.nodes[m].pos.y;
            if (nodeOpen[m]) hasOpen = true;
            if (nodeRoom[m]) hasRoom = true;
            floorZSum += nodeFloorZ[m];
            ++size;
        }
        if (size > 0) {
            centroid.x /= size;
            centroid.y /= size;
            centroid.z /= size;
        }
        float centroidFloorZ = (size > 0) ? floorZSum / size : 0.0f;
        if (size > 1) ++multiNodeClusters;

        // Longer bounding-box side = footprint extent. Single-node
        // clusters have zero extent and can never be "large". The
        // shorter side feeds the corridor-zone demotion (a run is long
        // AND narrow; a place is big both ways).
        float bboxExtent  = 0.0f;
        float bboxMinSide = 0.0f;
        if (size > 0) {
            float w = maxX - minX;
            float h = maxY - minY;
            bboxExtent  = (w > h) ? w : h;
            bboxMinSide = (w > h) ? h : w;
        }
        bool isLargeArea = bboxExtent >= kLargeAreaExtentMeters;

        // Per-axis extent of the cluster = median over its members' own
        // 8-ray extents. Feeds the elongation verdict (ComputeCentroidAxis
        // explains why one centroid probe is not enough). Median, not
        // mean: a member sitting on a crossing corridor reads tens of
        // metres along that corridor and must not drag the verdict.
        float axisExt[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bool haveAxisExt = false;
        if (size > 0 && bp.haveGW) {
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
                haveAxisExt = true;
            }
        }

        // External neighbours: edges from any member to a non-member.
        // Dedup by node id so multi-edge connections to the same
        // outside node only register once. Capture the source cluster
        // member alongside the nb so ClassifyCluster's door-on-edge
        // test can use the ACTUAL graph edge (member.pos → nb.pos)
        // rather than the centroid → nb.pos blur. For multi-node Platz
        // clusters the centroid is a synthesized midpoint between
        // members; testing perpendicular distance from that midpoint
        // matches doors that aren't on any real graph edge, which is
        // why the Upper-City plaza was lighting up with "Tür Nord, Tür
        // Ost, Tür Süd, Tür West" all at once.
        //
        // Edge-class filtering: each member→nb graph edge runs through
        // ClassifyEdge (the unified wall + door primitive). Blocked
        // edges drop entirely — the graph offered an exit, but the
        // walkmesh says it crosses a wall, so the cluster simply
        // doesn't see that neighbour. Door verdicts are cached in
        // externalDoorIdx[] so ClassifyCluster doesn't run a second
        // door query per neighbour.
        constexpr int kMaxExternal = 16;
        int externalNbs[kMaxExternal];
        int externalSrcs[kMaxExternal];   // cluster member that owns this edge
        int externalDoorIdx[kMaxExternal];
        int externalCount = 0;
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            int lo = 0, hi = 0;
            acc::engine::navgraph::NeighbourRange(g, m, lo, hi);
            for (int e = lo; e < hi; ++e) {
                int nb = static_cast<int>(g.conns[e]);
                if (nb < 0 || nb >= n) continue;
                if (UFFind(nb) == root) continue;  // internal edge
                EdgeResult er = ClassifyEdge(area,
                                             g.nodes[m].pos, g.nodes[nb].pos,
                                             "ext-edge");
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
                for (int k = 0; k < externalCount; ++k) {
                    if (externalNbs[k] == nb) {
                        // Upgrade -1 doorIdx if a later edge to the same
                        // neighbour found a door the first didn't.
                        if (externalDoorIdx[k] < 0 &&
                            er.kind == kEdgeDoor) {
                            externalDoorIdx[k] = er.doorIdx;
                        }
                        seen = true;
                        break;
                    }
                }
                if (!seen) {
                    if (externalCount < kMaxExternal) {
                        externalNbs    [externalCount] = nb;
                        externalSrcs   [externalCount] = m;
                        externalDoorIdx[externalCount] =
                            (er.kind == kEdgeDoor) ? er.doorIdx : -1;
                        ++externalCount;
                    } else {
                        // Cap hit — this exit won't be voiced. Log it rather
                        // than drop silently so an under-reported plaza is
                        // visible in the harvest (raise kMaxExternal if real).
                        acclog::Write(
                            "WallTopo",
                            "  external CAP: cluster=%d already has "
                            "kMaxExternal=%d neighbours — dropping exit to "
                            "nb[%d] (%.1f,%.1f); this exit will be missing "
                            "from the label",
                            root, kMaxExternal, nb,
                            g.nodes[nb].pos.x, g.nodes[nb].pos.y);
                    }
                }
            }
        }

        // Announce decision — now independent of how the cluster merged.
        // A cluster speaks as "Bereich" only when the clearance probe says
        // it is a PLACE: any open-class member, a multi-node room cluster,
        // or an isolated cluster with no exits. Everything else takes the
        // graph path and is voiced by its exit topology — a tight merged
        // hub with no open/room geometry reads as "Kreuzung", a degree-2
        // run as "Korridor", a single exit as "Sackgasse". This is what
        // keeps junctions sounding like junctions and corridors like
        // corridors even though they merge by the same space rule.
        //
        // The old "size>1 && externalCount>=3 → area" clause was dropped on
        // purpose: a merged corridor-junction hub (space-class by degree
        // alone) must stay a Kreuzung, not become a Bereich.
        //   - any open-class member       → big area (delayed)
        //   - multi-node room cluster      → room  (immediate)
        //   - isolated cluster (no exits)  → area (open/room by flag)
        int areaHint = 0;  // 0 none, 1 room, 2 big
        bool isArea = (externalCount == 0) || hasOpen ||
                      (size > 1 && hasRoom);
        if (isArea) areaHint = (hasRoom && !hasOpen) ? 1 : 2;

        // Zone-scale refinement of that dropped clause: a junction web
        // whose footprint exceeds kJunctionZoneExtentMeters is a place,
        // not a crossing — render it as a Bereich (see the constant's
        // comment for the Lehon evidence). externalCount >= 3 keeps
        // long merged corridor CHAINS (2 exits, routinely >15m) and
        // dead-end runs out of the override; small merged hubs stay
        // Kreuzung per the original decision.
        // Zone-scale rule, one statement for every exit count: a cluster
        // whose member footprint is zone-scale is a PLACE, and renders
        // through the area path ("Bereich [axis]. Ausgänge: …") rather
        // than the crossing / run / cul-de-sac vocabulary, which all
        // promise something a few steps wide.
        //
        // Two ways to qualify, and the difference is only about what
        // protects long hallways:
        //   - 3+ exits: a junction web that big is a plaza whatever its
        //     shape (Lehon Südlicher Strand, 46m, re-announced as a new
        //     crossing from every fringe channel).
        //   - 1-2 exits: must be big in BOTH dimensions, because a real
        //     corridor is long and NARROW (2.5-3m in vanilla content) —
        //     the min-side test is what keeps hallways sounding like
        //     hallways. Covers the one-exit case too: the Ebon-Hawk
        //     Backbord deck is a 34-node field with a single ramp out,
        //     and calling that "Sackgasse" hid a space you walk around
        //     in — you have to cross it to reach the Protonenraketen.
        if (areaHint == 0 && bboxExtent >= kJunctionZoneExtentMeters &&
            (externalCount >= 3 || bboxMinSide >= kZoneMinWidthM)) {
            areaHint = 2;
            acclog::Write("WallTopo",
                          "  zone-scale: cluster=%d size=%d extent=%.1fm "
                          "minside=%.1fm exits=%d (>= %.1fm, and 3+ exits "
                          "or >= %.1fm wide) — rendered as Bereich",
                          root, size, bboxExtent, bboxMinSide,
                          externalCount, kJunctionZoneExtentMeters,
                          kZoneMinWidthM);
        }

        // Transition triggers whose nearest nav node is a member of this
        // cluster become named exits in its label.
        constexpr int kMaxClusterTransitions = 4;
        int trigIdxs[kMaxClusterTransitions];
        int trigCount = 0;
        for (int t = 0; t < g_graph.transition_count &&
                        trigCount < kMaxClusterTransitions; ++t) {
            int nn = g_graph.transitions[t].nearestNode;
            if (nn >= 0 && nn < n && UFFind(nn) == root) {
                trigIdxs[trigCount++] = t;
            }
        }

        // Dead-end verdict for THIS cluster, resolved from its terminal
        // members (the degree-1 ones — where a terminal wall would be)
        // rather than from the centroid, which for a multi-node run
        // sits mid-corridor. Any confirming terminal is enough; a
        // cluster with no degree-1 member at all can't be judged, so it
        // is not filtered.
        bool terminalDeadEndAgrees = true;
        {
            bool sawTerminal = false, anyAgrees = false;
            for (int m = 0; m < n; ++m) {
                if (UFFind(m) != root) continue;
                if (Degree(g, m) != 1) continue;
                sawTerminal = true;
                if (bp.nodeDeadEnd[m]) { anyAgrees = true; break; }
            }
            if (sawTerminal) terminalDeadEndAgrees = anyAgrees;
        }

        // Floor footprint for announce timing (ClusterInfo::footprintM).
        // Node spread alone reads a single-node crossing as zero metres
        // wide; the narrowest wall-to-wall axis through the members is
        // the crossing's real width (an apartment-hall T junction on
        // Taris: ~11 m, node spread 0).
        float footprint = bboxExtent;
        if (haveAxisExt) {
            float narrowest = axisExt[0];
            for (int a = 1; a < 4; ++a)
                if (axisExt[a] < narrowest) narrowest = axisExt[a];
            if (narrowest > footprint) footprint = narrowest;
        }

        std::string label;
        int kind = kKindOpenArea, sig = 0;
        bool filtered = false;
        float bboxXY[4] = {minX, minY, maxX, maxY};
        std::vector<ExitMark> exits;
        g_exit_sink = &exits;
        ClassifyCluster(g, centroid, externalNbs, externalSrcs,
                        externalDoorIdx, externalCount,
                        trigIdxs, trigCount,
                        areaHint, centroidFloorZ, isLargeArea,
                        (size > 0) ? bboxXY : nullptr,
                        haveAxisExt ? axisExt : nullptr,
                        bp.nodeDeadEnd, terminalDeadEndAgrees,
                        label, kind, sig, filtered);
        g_exit_sink = nullptr;

        switch (kind) {
            case kKindDeadEnd:  ++deadEnds;  break;
            case kKindCorridor: ++corridors; break;
            case kKindJunction: ++junctions; break;
            case kKindPlatz:    ++junctions; break;  // tally w/ junctions
            case kKindRoom:     ++junctions; break;  // tally w/ junctions
            default:            ++openAreas; break;
        }

        if (isLargeArea) {
            acclog::Write("WallTopo",
                          "  large-area: cluster=%d size=%d extent=%.1fm "
                          "(>= %.1fm) -> '%s'",
                          root, size, bboxExtent, kLargeAreaExtentMeters,
                          label.c_str());
        }

        // Write to every cluster member.
        for (int m = 0; m < n; ++m) {
            if (UFFind(m) != root) continue;
            g_graph.node_label[m]    = label;
            g_graph.node_exits[m]    = exits;
            g_graph.node_kind[m]     = kind;
            g_graph.node_sig[m]      = sig;
            g_graph.node_filtered[m] = filtered;
            g_graph.node_extent[m]   = bboxExtent;
            g_graph.node_centre[m]   = centroid;
            g_graph.node_footprint[m] = footprint;
        }
    }
    bp.clusters          = clusters;
    bp.multiNodeClusters = multiNodeClusters;
    bp.deadEnds          = deadEnds;
    bp.corridors         = corridors;
    bp.junctions         = junctions;
    bp.openAreas         = openAreas;
}

}  // namespace acc::room_topology
