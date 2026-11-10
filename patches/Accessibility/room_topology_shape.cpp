// The shape of nodes and regions: per-node features from the 8-ray wall
// probe (open, room, floor height, dead-end verdict, axis extents), the long
// axis of a region, and the octants an axis word covers.

#include "room_topology_internal.h"

#include "log.h"
#include "spatial_wall_surfaces.h"  // GetCachedWalls: the seam-filtered perimeter cache

namespace acc::room_topology {

namespace {

// A ray past this is an arm toward an exit at corridor scale (corridor
// nodes between close junctions never reach the 8 m open scale). Logged
// only.
constexpr float kStructRayDistM = 5.0f;

// A node is a room when it sits in a bounded, evenly open space: at most
// one ray clears the open scale, none passes kRoomBoundedMaxM, and at least
// kRoomMinMidRays land between kRoomNearM (closer is wall-hugging) and the
// open scale. The evenness separates a room interior from a corner or
// doorway node (mostly short rays plus one escape). It marks the node as
// space for the merge passes; it fires in small rooms (Ebon Hawk 21,
// Apartments 9) and not outdoors (Slums 3).
constexpr float kRoomNearM       = 2.0f;
constexpr float kRoomBoundedMaxM = 10.0f;
constexpr int   kRoomMinMidRays  = 3;

// A region is elongated, and speaks its long axis ("Area East-West"), when
// its longest axis exceeds this times the perpendicular one; square, round
// and open spaces speak a bare "Area".
constexpr float kAxisElongRatio = 1.8f;

// The tentative per-node class, for the clearance log only: the connector
// classes count real long arms (open-scale rays), rooms go by evenness.
const char* ShapeClass(int openRays, int longBits, bool room) {
    if (openRays >= kOpenMinRays) return "open";
    if (openRays >= 3) return "junction";
    if (openRays == 2) {
        int b0 = -1, b1 = -1;
        for (int k = 0; k < 8; ++k) {
            if (longBits & (1 << k)) {
                if (b0 < 0) b0 = k; else b1 = k;
            }
        }
        int d = (b0 >= 0 && b1 >= 0) ? (b1 - b0) : 0;
        if (d > 4) d = 8 - d;
        return (d >= 3) ? "corridor" : "corner";
    }
    if (openRays == 1) return room ? "room" : "deadend";
    return room ? "room" : "corner";
}

void LogAxisVerdict(const Vector& centroid, const float ext[4], bool fromMembers, bool elong,
                    int longA) {
    static const char* const kAxisNames[4] = {"E-W", "NE-SW", "N-S", "NW-SE"};
    acclog::Write("WallTopo", "ComputeCentroidAxis: at (%.1f,%.1f) %s extents "
                  "E-W=%.1f NE-SW=%.1f N-S=%.1f NW-SE=%.1f -> %s",
                  centroid.x, centroid.y, fromMembers ? "member-median" : "centroid-probe",
                  ext[0], ext[1], ext[2], ext[3], elong ? kAxisNames[longA] : "no axis");
}

}  // namespace

int Degree(const acc::engine::navgraph::NavGraphSnapshot& g, int node) {
    int lo = 0, hi = 0;
    acc::engine::navgraph::NeighbourRange(g, node, lo, hi);
    return hi - lo;
}

// Runs once at the top of BuildForArea, before any merge pass, so every
// pass sees the same flags; every ray and feature is logged so the
// thresholds stay re-pickable.
void ComputeNodeShapeFeatures(const acc::engine::navgraph::NavGraphSnapshot& g,
                              int n, bool* nodeOpen, bool* nodeRoom,
                              float* nodeFloorZ, bool* nodeDeadEnd,
                              float (*nodeAxisExt)[4]) {
    for (int i = 0; i < n; ++i) {
        nodeOpen[i] = false;
        nodeRoom[i] = false;
        nodeFloorZ[i] = 0.0f;
        nodeDeadEnd[i] = false;
        for (int a = 0; a < 4; ++a) nodeAxisExt[i][a] = 0.0f;
    }
    const acc::engine::WallEdge* cw = nullptr;
    int cwCount = 0;
    if (!acc::spatial::wall_surfaces::GetCachedWalls(cw, cwCount) || !cw || cwCount <= 0) {
        acclog::Write("WallTopo", "  clearance-dump: no wall cache — open/room coalesce "
                      "skipped (no shape data)");
        return;
    }
    acclog::Write("WallTopo",
                  "  clearance-dump: per-node 8-ray shape (feeds open + room coalesce)");
    for (int i = 0; i < n; ++i) {
        // Nodes sit at z=0 while walls sit at the floor, where a z=0 ray
        // would pass under every wall: the probe takes the height-band vote
        // of the nearby walls (a single nearest endpoint once latched onto
        // an anomaly edge 4 m overhead and called a dead-end pocket open).
        Vector probePos = g.nodes[i].pos;
        probePos.z = acc::wall_probe::ResolveProbeZ(cw, cwCount, probePos);
        nodeFloorZ[i] = probePos.z;
        float r[8];
        ProbeClearance8(cw, cwCount, probePos, r);
        // The full extent across each axis (0 E-W, 1 NE-SW, 2 N-S, 3 NW-SE),
        // kept per node so Pass 3 judges a cluster's elongation from all its
        // members (ComputeCentroidAxis).
        for (int a = 0; a < 4; ++a) nodeAxisExt[i][a] = r[a] + r[a + 4];

        float mn = r[0], mx = r[0];
        int mnBit = 0, mxBit = 0;
        int openRays = 0, structN = 0, midN = 0, shortN = 0;
        int longBits = 0, structBits = 0;
        float sum = 0.0f;
        for (int k = 0; k < 8; ++k) {
            if (r[k] < mn) { mn = r[k]; mnBit = k; }
            if (r[k] > mx) { mx = r[k]; mxBit = k; }
            sum += r[k];
            if (r[k] > kOpenRayDistM)   { ++openRays; longBits   |= (1 << k); }
            if (r[k] > kStructRayDistM) { ++structN;  structBits |= (1 << k); }
            if (r[k] >= kRoomNearM && r[k] < kOpenRayDistM) ++midN;
            if (r[k] < kRoomNearM) ++shortN;
        }
        nodeOpen[i] = openRays >= kOpenMinRays;
        nodeRoom[i] = openRays <= 1 && mx <= kRoomBoundedMaxM && midN >= kRoomMinMidRays;

        acclog::Write(
            "WallTopo",
            "    clearance node[%d] (%.1f,%.1f) deg=%d probeZ=%.1f "
            "min=%.1f@%d max=%.1f@%d mean=%.1f openN=%d structN=%d "
            "midN=%d shortN=%d open=%d room=%d structbits=0x%02x class=%s "
            "rays[E,NE,N,NW,W,SW,S,SE]=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            i, g.nodes[i].pos.x, g.nodes[i].pos.y, Degree(g, i), probePos.z,
            mn, mnBit, mx, mxBit, sum / 8.0f, openRays, structN,
            midN, shortN, nodeOpen[i] ? 1 : 0, nodeRoom[i] ? 1 : 0,
            structBits, ShapeClass(openRays, longBits, nodeRoom[i]),
            r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
    }

    // Does the walkmesh agree that a degree-1 node ends a region? The nav
    // graph is BioWare's hand-made AI patrol graph, with degree-1 nodes at
    // wall bumps, room corners and patrol ends that no player can walk
    // into. A real alcove has walls on three sides and its entrance on the
    // fourth, probed along the node's own graph edge (a diagonal alcove
    // reads as a junction under compass rays) and from the node, not a
    // cluster centroid, which sits mid-run. Asked here, before the merges,
    // so they can use it. Missing wall data fails open (trust the graph).
    for (int i = 0; i < n; ++i) {
        if (Degree(g, i) != 1) continue;
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        if (lo >= hi) continue;
        const int nb = static_cast<int>(g.conns[lo]);
        if (nb < 0 || nb >= n) continue;
        Vector p = g.nodes[i].pos;
        p.z = nodeFloorZ[i];
        nodeDeadEnd[i] = IsAlcoveAlongAxis(p, g.nodes[nb].pos.x - p.x, g.nodes[nb].pos.y - p.y);
        if (!nodeDeadEnd[i]) {
            acclog::Write("WallTopo", "    deadend-verdict node[%d] (%.1f,%.1f) -> "
                          "NOT a real dead end (walkmesh continues past it)",
                          i, g.nodes[i].pos.x, g.nodes[i].pos.y);
        }
    }
}

// memberExt holds the cluster's per-axis extents, each the median over its
// members' probes. A single probe at the centroid misleads: a corridor's
// centroid sits mid-run, which is where a crossing corridor opens (Citadel
// Station's residential module 082 East: 11 nodes spanning 23 m east-west,
// but the centroid's north-south rays ran 50 m through a door, so "Area
// North-South" for a corridor walked east-west). nullptr falls back to that
// centroid probe (no per-node data).
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
        if (!acc::spatial::wall_surfaces::GetCachedWalls(cw, cwCount) || !cw || cwCount <= 0) {
            return false;
        }
        Vector p = centroid;
        p.z = floorZ;
        float r[8];
        ProbeClearance8(cw, cwCount, p, r);
        for (int a = 0; a < 4; ++a) ext[a] = r[a] + r[a + 4];
    }
    int longA = 0;
    for (int a = 1; a < 4; ++a) if (ext[a] > ext[longA]) longA = a;
    const int perpA = (longA + 2) % 4;
    // ext > 0: all-zero extents (no per-node data) must not claim East-West.
    const bool elong = ext[longA] > 0.0f && ext[longA] >= kAxisElongRatio * ext[perpA];
    LogAxisVerdict(centroid, ext, memberExt != nullptr, elong, longA);
    if (!elong) return false;
    switch (longA) {
        case 0: outAxisId = Id::AxisEastWest;   break;
        case 2: outAxisId = Id::AxisNorthSouth; break;
        case 1: outAxisId = BitToOctant(1);     break;  // NE-SW speaks "North-East"
        case 3: outAxisId = BitToOctant(3);     break;  // NW-SE speaks "North-West"
    }
    return true;
}

// The two opposite octant bits an axis word covers, or 0. An "East-West"
// area whose only exits are East and West speaks the axis alone.
int AxisOctantMask(acc::strings::Id axisId) {
    using acc::strings::Id;
    if (axisId == Id::AxisEastWest)   return (1 << 0) | (1 << 4);
    if (axisId == Id::AxisNorthSouth) return (1 << 2) | (1 << 6);
    if (axisId == Id::DirNortheast)   return (1 << 1) | (1 << 5);
    if (axisId == Id::DirNorthwest)   return (1 << 3) | (1 << 7);
    return 0;
}

}  // namespace acc::room_topology
