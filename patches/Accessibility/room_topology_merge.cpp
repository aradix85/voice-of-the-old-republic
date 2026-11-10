// Merging nav nodes into regions: union-find, the straightness of node
// chains, and the first two merge passes (core merge, corner fold).

#include "room_topology_internal.h"

#include "log.h"

namespace acc::room_topology {

namespace {

// Pass 1 / corridor rule — collapse straight degree-2 chain runs
// into one cluster. K1 corridors are authored as 5-15 degree-2 patrol
// nodes ~3-5m apart along a single axis; without merging each becomes
// its own cluster and a 15m hallway re-announces "corridor East-West"
// 3+ times as the player walks it (one corridor label was heard 13 times).
//
// A node is "straight" iff its two outgoing edge vectors point in
// roughly opposite directions (cos(angle) <= kCorridorStraightCosMax).
// Two adjacent straight degree-2 nodes union. Sharp bends (~90° kinks)
// fail the straightness test and break the chain — an L stays as two
// corridors meeting at the bend, which matches the perceptual model (the
// player feels the corridor turn, so it reads as two directions).
//
// Tolerance is +/- 35 deg of straight (cos -0.82): loose enough to keep a
// gently curving hall as ONE corridor (a series of ~30 deg patrol-node
// bends used to shatter into one cluster per node — the Ebon Hawk
// crew-quarters approach split into four), tight enough that a real ~90 deg
// corner still breaks. The +/-35 cut also tracks navigability: in a narrow
// hall a bend past ~35 deg drifts you into the wall if you hold the end
// bearing, so those genuinely need to stay separate turn cues. Tune from
// log evidence — raise toward -0.9 (+/-25 deg) if gentle curves over-merge,
// drop toward -0.7 (+/-45 deg) if real halls still fragment.
//
// A *sharp* bend isn't left as its own one-node cluster, though: the
// corner-fold step (right after the Pass 1 core merge) folds it into the
// adjacent corridor it continues straightest into — its straight edge
// extends that corridor, its turning edge becomes the boundary to the next
// one. So an L reads as corridor-A then corridor-B (two axes meeting at the
// turn), never as a lone "corner" announcement wedged between them.
//
// Gates beyond per-node straightness (shared by every Pass 1 rule):
//   - door-on-edge veto (preserves authored room boundaries);
//   - wall-on-edge veto (cheap insurance — shouldn't fire on real
//     corridor neighbours, but covers degree-2 nodes the engine
//     places across a wall in adjacency tables).
// A straight node adjacent to a junction hub can't merge into it: the hub
// is space (degree-≥3 or open/room) and a corridor node is passage, and
// passage↔space never merge, so the corridor stays a distinct core.
constexpr float kCorridorStraightCosMax = -0.82f;

}  // namespace

int UFFind(int x) {
    while (s_uf_parent[x] != x) {
        s_uf_parent[x] = s_uf_parent[s_uf_parent[x]];  // path halving
        x = s_uf_parent[x];
    }
    return x;
}

void UFUnite(int a, int b) {
    int ra = UFFind(a);
    int rb = UFFind(b);
    if (ra == rb) return;
    // Use smaller index as the cluster root so the dump output is
    // stable across runs (the root is also used as the cluster's id).
    if (ra < rb) s_uf_parent[rb] = ra;
    else         s_uf_parent[ra] = rb;
}

// Corridor straightness: a degree-2 node whose two edges point roughly
// opposite. Precomputed so the corridor rule is a both-straight test in
// the edge-walk; also reused by Pass 2 to keep corridor material out of
// the straggler absorb, and by the corner-fold step (chainStraightCos
// lets it pick the straightest neighbour to fold a bend into).
void ComputeChainStraightness(const acc::engine::navgraph::NavGraphSnapshot& g,
                              int n, BuildPasses& bp) {
    bool*  chainStraight    = bp.chainStraight;
    float* chainStraightCos = bp.chainStraightCos;
    for (int i = 0; i < n; ++i) {
        chainStraight[i]    = false;
        chainStraightCos[i] = 1.0f;
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        if (hi - lo != 2) continue;
        int nA = static_cast<int>(g.conns[lo]);
        int nB = static_cast<int>(g.conns[lo + 1]);
        if (nA < 0 || nA >= n || nB < 0 || nB >= n || nA == nB) continue;
        float ax = g.nodes[nA].pos.x - g.nodes[i].pos.x;
        float ay = g.nodes[nA].pos.y - g.nodes[i].pos.y;
        float bx = g.nodes[nB].pos.x - g.nodes[i].pos.x;
        float by = g.nodes[nB].pos.y - g.nodes[i].pos.y;
        float la = std::sqrt(ax * ax + ay * ay);
        float lb = std::sqrt(bx * bx + by * by);
        if (la < 1e-3f || lb < 1e-3f) continue;
        chainStraightCos[i] = (ax * bx + ay * by) / (la * lb);
        if (chainStraightCos[i] <= kCorridorStraightCosMax)
            chainStraight[i] = true;
    }
}

// ===== Pass 1: core merge =====
// Form perceptual cores by walking the nav graph once and unioning each
// clear, door-free edge whose endpoints are the SAME kind of thing.
// Internally there are now only two kinds — the merge layer no longer
// separates junction / room / open (they were the same operation,
// "union adjacent same-class nodes", and the open-vs-room seam left
// door-less interiors fragmented: an Ebon Hawk hold split open↔throat↔
// room into three regions). Collapsed to:
//
//   - SPACE   : a place the player stands in — degree-≥3 hub, or open /
//               room by the clearance probe. Two space nodes union.
//   - PASSAGE : a straight degree-2 link. Two straight nodes union into
//               one corridor run (the L-shaped-corridor rule is carried
//               over unchanged: a ~90° bend fails the straightness test
//               and breaks the run, so the player still hears the turn).
//
// PASSAGE↔SPACE never merge — that boundary is exactly the line between
// "I'm in a corridor" and "I'm in a room", and is what lets Pass 3 still
// voice corridors, junctions and areas distinctly even though the merge
// itself stopped distinguishing them.
//
// Each rule is gated on a clear, door-free edge: the door is tested in
// 2D (FindDoorOnEdge); the wall is tested on the floor-z-lifted segment
// (the nav nodes sit at z=0, so a z=0 segment would run under same-floor
// walls). Adjacency — a real engine nav edge — is the connector, so two
// spaces that merely face each other across a junction throat stay
// separate (the Upper City store / cantina-avenue fix).
void PassCoreMerge(const acc::engine::navgraph::NavGraphSnapshot& g,
                   int n, BuildPasses& bp) {
    const bool*  nodeOpen         = bp.nodeOpen;
    const bool*  nodeRoom         = bp.nodeRoom;
    const float* nodeFloorZ       = bp.nodeFloorZ;
    const bool*  chainStraight    = bp.chainStraight;
    const acc::engine::WallEdge* gw = bp.gw;
    const int  gwCount = bp.gwCount;
    const bool haveGW  = bp.haveGW;

    int coreSpace = 0, coreCorridor = 0;
    int coreVetoDoor = 0, coreVetoWall = 0;
    // Unified space cap. The merge layer no longer separates junction (was
    // 8m) / room (8m) / open (16m); one generous adjacency cap covers all
    // three. A real nav edge plus the door + wall vetoes keep it honest, so
    // the 16m reach can't bridge a wall or an authored doorway.
    const float kSpaceCapSq = kOpenMergeRadiusM * kOpenMergeRadiusM;
    for (int i = 0; i < n; ++i) {
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, i, lo, hi);
        int degI = hi - lo;
        for (int e = lo; e < hi; ++e) {
            int j = static_cast<int>(g.conns[e]);
            if (j <= i || j >= n) continue;        // each undirected pair once
            if (UFFind(i) == UFFind(j)) continue;
            float dx = g.nodes[i].pos.x - g.nodes[j].pos.x;
            float dy = g.nodes[i].pos.y - g.nodes[j].pos.y;
            float dz = g.nodes[i].pos.z - g.nodes[j].pos.z;
            if (std::fabs(dz) > kMergeMaxZM) continue;   // multi-floor guard
            float d2 = dx * dx + dy * dy;

            // Decide the core kind before running the edge tests. Space =
            // degree-≥3 hub or open / room by the probe; passage = straight
            // degree-2. Space unions with space (one cap); straight unions
            // with straight (no cap — a chain collapses fully). Passage↔space
            // falls through and stays unmerged.
            int degJ = Degree(g, j);
            bool spaceI = (degI >= 3) || nodeOpen[i] || nodeRoom[i];
            bool spaceJ = (degJ >= 3) || nodeOpen[j] || nodeRoom[j];
            const char* kind = nullptr;
            if (spaceI && spaceJ && d2 <= kSpaceCapSq)         kind = "space";
            else if (chainStraight[i] && chainStraight[j])     kind = "corridor";
            if (!kind) continue;

            if (FindDoorOnEdge(g.nodes[i].pos, g.nodes[j].pos) >= 0) {
                ++coreVetoDoor;
                continue;
            }
            if (haveGW) {
                Vector a = g.nodes[i].pos, b = g.nodes[j].pos;
                float fz = 0.5f * (nodeFloorZ[i] + nodeFloorZ[j]);
                a.z = fz; b.z = fz;
                Vector hit{};
                if (acc::engine::SegmentCrossesWalkmesh(gw, gwCount, a, b, hit)) {
                    ++coreVetoWall;
                    continue;
                }
            }

            UFUnite(i, j);
            switch (kind[0]) {
                case 's': ++coreSpace;    break;
                default:  ++coreCorridor; break;
            }
            acclog::Write(
                "WallTopo",
                "  core-merge [%s]: node[%d] (%.1f,%.1f) + node[%d] (%.1f,%.1f) "
                "gap=%.1fm",
                kind, i, g.nodes[i].pos.x, g.nodes[i].pos.y,
                j, g.nodes[j].pos.x, g.nodes[j].pos.y, std::sqrt(d2));
        }
    }
    acclog::Write(
        "WallTopo",
        "  core-merge: space=%d corridor=%d "
        "vetoedByDoor=%d vetoedByWall=%d",
        coreSpace, coreCorridor,
        coreVetoDoor, coreVetoWall);
    bp.coreSpace    = coreSpace;
    bp.coreCorridor = coreCorridor;
}

// ===== Pass 1b: corner fold =====
// A sharp bend (degree-2, not chainStraight) is the corner where two
// corridor segments meet. Left alone it announces as its own one-node
// cluster wedged between the two runs (the Ebon Hawk crew-quarters
// 51° dogleg: "corridor / corner / corridor", three cues). Fold it into
// the neighbour it continues STRAIGHTEST into — its straight edge extends
// that corridor, its turning edge becomes the boundary to the next one.
//
// "Straightest" = the neighbour nb with the most-opposite edges (smallest
// chainStraightCos), among the corner's degree-2 chainStraight neighbours
// reachable across a clear, door-free edge. Folding into exactly one side
// means the corner NEVER unions across its own bend, so the two corridors
// stay distinct (each announced by its own axis) and the player turns at
// the boundary — no merged corridor with a misleading end bearing. Runs
// before Pass 2 so a corner prefers its straight corridor over being
// absorbed into an adjacent space hub. Single pass: folding a corner
// can't create a new corner.
void PassCornerFold(const acc::engine::navgraph::NavGraphSnapshot& g,
                    int n, BuildPasses& bp) {
    const float* nodeFloorZ       = bp.nodeFloorZ;
    const bool*  chainStraight    = bp.chainStraight;
    const float* chainStraightCos = bp.chainStraightCos;
    const acc::engine::WallEdge* gw = bp.gw;
    const int  gwCount = bp.gwCount;
    const bool haveGW  = bp.haveGW;

    int cornerFolds = 0;
    for (int x = 0; x < n; ++x) {
        int lo = 0, hi = 0;
        acc::engine::navgraph::NeighbourRange(g, x, lo, hi);
        if (hi - lo != 2) continue;        // degree-2 only
        if (chainStraight[x]) continue;    // straight nodes already chained
        int   bestNb  = -1;
        float bestCos = 2.0f;              // smaller = straighter pass-through
        for (int e = lo; e < hi; ++e) {
            int nb = static_cast<int>(g.conns[e]);
            if (nb < 0 || nb >= n || nb == x) continue;
            if (!chainStraight[nb]) continue;   // join must continue straight
            if (UFFind(nb) == UFFind(x)) continue;
            if (FindDoorOnEdge(g.nodes[x].pos, g.nodes[nb].pos) >= 0) continue;
            if (haveGW) {
                Vector a = g.nodes[x].pos, b = g.nodes[nb].pos;
                float fz = 0.5f * (nodeFloorZ[x] + nodeFloorZ[nb]);
                a.z = fz; b.z = fz;
                Vector hit{};
                if (acc::engine::SegmentCrossesWalkmesh(gw, gwCount, a, b, hit))
                    continue;
            }
            if (chainStraightCos[nb] < bestCos) {
                bestCos = chainStraightCos[nb];
                bestNb  = nb;
            }
        }
        if (bestNb >= 0) {
            acclog::Write(
                "WallTopo",
                "  corner-fold: node[%d] (%.1f,%.1f) bend-cos=%.2f -> corridor "
                "via node[%d] (%.1f,%.1f) straight-cos=%.2f",
                x, g.nodes[x].pos.x, g.nodes[x].pos.y, chainStraightCos[x],
                bestNb, g.nodes[bestNb].pos.x, g.nodes[bestNb].pos.y, bestCos);
            UFUnite(x, bestNb);
            ++cornerFolds;
        }
    }
    acclog::Write("WallTopo", "  corner-fold: folded=%d", cornerFolds);
}

}  // namespace acc::room_topology
