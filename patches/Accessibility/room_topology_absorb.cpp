// The later merge passes: small leftover nodes are absorbed into the
// region next to them (adjacent stragglers, stragglers inside a region's
// bounding box, open-ended pendants).

#include "room_topology_internal.h"

namespace acc::room_topology {

namespace {

// (The old density-based merge — pack ≥N nav nodes within 3m → one
// cluster — was retired 2026-06-09. It was a crude proxy for "small
// enclosed room"; the clearance probe now measures room-shape directly
// and the space rule subsumes it.)
// Pass 2 / straggler absorb — folds a degree-≤2 corner/spoke into an
// adjacent multi-node core (the merged successor of the old hub + bbox
// absorption passes). It targets the K1 "two-hub star" small-room pattern
// (Endar Spire quarters: two deg-4 nodes merge into a core, but the
// surrounding degree-1/2 patrol spokes would label independently — 5
// shape readings as the player walks) and the building-corner stragglers
// in open areas. kStragglerAbsorbMaxM is a sanity cap on the corner→core
// edge length — adjacency (a real nav edge) is the connector, not
// distance; 16m covers the Oberstadt store corners (9.9m off the frontage)
// with headroom while excluding cross-plaza edges. Tune from harvested
// absorb logs. (The target-must-be-multi-node and door-veto gates that
// keep corridors from being eaten live in the pass body.)
constexpr float kStragglerAbsorbMaxM    = 16.0f;

// ===== Pass 2c: open-ended pendant absorb =====
//
// Consumes the per-node dead-end verdict (see ComputeNodeShapeFeatures).
// A cluster that hangs off exactly ONE other cluster is a pendant: the
// graph says it is a region you enter and leave the same way. That is
// only true if it actually ENDS. When its terminal degree-1 members are
// all walkmesh-rejected — the mesh keeps going past them, they just
// have no path points beyond — the pendant is not a region of its own;
// it is part of the space it hangs off, and it folds in.
//
// This is what the "filtered / rescue-only" flag could not do from
// Pass 3: by then clustering was frozen, so a false dead-end could only
// be muted, never un-clustered. Three interconnected Ebon-Hawk hull
// lanes stood as silent ghost "Sackgasse" regions for exactly that
// reason, while the crossing they hang off advertised a northern exit
// that only loops back into them.
//
// Distance guard (kPendantAbsorbMaxM): fold only into a space that is
// actually adjacent. The rescue path exists for a documented Taris
// case (tar_m11aa, room-shape-improvements.md "Bucket 3") where the
// player stood 0.8m from a rejected node whose nearest labelled cluster
// was 23.8m away — folding THAT into a region across the building is
// the "wrong label across the map" failure the same doc warns about.
// Hull lanes sit ~6m off their crossing, so the cap separates the two
// cases with a wide margin at both ends; beyond it a rejected pendant
// stays its own rescue-only cluster exactly as before.
//
// Cascades (a fold can make the survivor a pendant in turn) under the
// same iteration guard the straggler absorb uses.
constexpr float kPendantAbsorbMaxM = 11.0f;

}  // namespace

// ===== Pass 2: straggler absorb =====
// Fold leftover corner / doorway nodes into the core they belong to.
// The merged successor of the old hub-absorption + bbox-absorption
// passes, run HERE (after every core merge, including open) so a corner
// hanging off a plaza folds into the plaza, not a singleton. Two
// admission rules, both preserving authored boundaries via the door
// veto:
//
//   (a) graph-attached: a degree-≤2 singleton that is NOT corridor
//       material (chainStraight) folds into an adjacent multi-node core
//       across a clear, door-free edge within kStragglerAbsorbMaxM.
//       Cascades to a fixpoint on LIVE membership, so a corner two hops
//       from a core still folds in (Oberstadt store corner 24 → spoke
//       14 → frontage core — the case the old hub pass structurally
//       missed, since it froze its snapshot before building the core).
//       Real corridors are safe: they are multi-node cores (Pass 1
//       chained them) and chainStraight singletons are excluded, so the
//       cascade can neither eat a corridor core nor propagate through a
//       straight corridor node.
//
//   (b) graph-unattached: a remaining singleton whose XY sits inside a
//       core's bounding box, with a member within kMergeMaxZM in Z and a
//       wall-/door-clear segment to it. Catches nodes that only connect
//       to door-vetoed neighbours (Endar Spire start node[3]/[6]).
// (a) Adjacency cascade.
void AbsorbAdjacentStragglers(const acc::engine::navgraph::NavGraphSnapshot& g,
                              int n, BuildPasses& bp) {
    const float* nodeFloorZ    = bp.nodeFloorZ;
    const bool*  chainStraight = bp.chainStraight;
    const acc::engine::WallEdge* gw = bp.gw;
    const int  gwCount = bp.gwCount;
    const bool haveGW  = bp.haveGW;

    int absorbAdj = 0, absorbAdjVetoDoor = 0, absorbAdjVetoWall = 0;
    {
        int sizeByRoot[kMaxNodes];
        const float kCapSq = kStragglerAbsorbMaxM * kStragglerAbsorbMaxM;
        bool changed = true;
        int  guard   = 0;
        while (changed && guard++ < n) {
            changed = false;
            for (int s = 0; s < n; ++s) sizeByRoot[s] = 0;
            for (int s = 0; s < n; ++s) {
                int r = UFFind(s);
                if (r >= 0 && r < n) ++sizeByRoot[r];
            }
            for (int x = 0; x < n; ++x) {
                if (Degree(g, x) > 2) continue;          // corner / spoke only
                if (chainStraight[x]) continue;          // corridor material
                if (sizeByRoot[UFFind(x)] >= 2) continue; // already in a core
                int lo = 0, hi = 0;
                acc::engine::navgraph::NeighbourRange(g, x, lo, hi);
                // Scan valid core-neighbours (clear, door-free, within cap)
                // and collect the distinct cores X reaches. A node that
                // reaches >= 2 distinct cores BRIDGES two areas — keep it as
                // its own announcing junction so both connections stay
                // audible (Oberstadt node 25 links the store frontage to the
                // cantina avenue; folding it into the avenue erased the
                // "Süd" exit cue and the avenue became unfindable from the
                // frontage). Only a pendant — exactly one reachable core —
                // folds in.
                int coreRootA = -1, coreNbA = -1;
                bool isBridge = false;
                for (int e = lo; e < hi && !isBridge; ++e) {
                    int j = static_cast<int>(g.conns[e]);
                    if (j < 0 || j >= n || j == x) continue;
                    int jr = UFFind(j);
                    if (sizeByRoot[jr] < 2) continue;  // neighbour is not a core
                    if (jr == UFFind(x)) continue;
                    if (DistXYSq(g.nodes[x].pos, g.nodes[j].pos) > kCapSq) continue;
                    if (FindDoorOnEdge(g.nodes[x].pos, g.nodes[j].pos) >= 0) {
                        ++absorbAdjVetoDoor;
                        continue;
                    }
                    if (haveGW) {
                        Vector a = g.nodes[x].pos, b = g.nodes[j].pos;
                        float fz = 0.5f * (nodeFloorZ[x] + nodeFloorZ[j]);
                        a.z = fz; b.z = fz;
                        Vector hit{};
                        if (acc::engine::SegmentCrossesWalkmesh(gw, gwCount,
                                                                a, b, hit)) {
                            ++absorbAdjVetoWall;
                            continue;
                        }
                    }
                    if (coreRootA < 0) { coreRootA = jr; coreNbA = j; }
                    else if (jr != coreRootA) isBridge = true;
                }
                if (isBridge) continue;        // keep bridge as its own junction
                if (coreRootA >= 0) {
                    acclog::Write(
                        "WallTopo",
                        "  absorb [adj]: node[%d] (%.1f,%.1f) deg=%d -> "
                        "core=%d (via node[%d] at %.1f,%.1f)",
                        x, g.nodes[x].pos.x, g.nodes[x].pos.y, Degree(g, x),
                        coreRootA, coreNbA,
                        g.nodes[coreNbA].pos.x, g.nodes[coreNbA].pos.y);
                    // Carry the combined size onto the merged set's NEW
                    // root. sizeByRoot is indexed by root id and only
                    // rebuilt per outer iteration, but UFUnite roots at the
                    // SMALLER index — so absorbing a low-id straggler into a
                    // higher-rooted core moves the root onto a node whose
                    // snapshot entry still reads 1. Every later "already in
                    // a core" test for that set then sees a singleton and
                    // the guard above silently fails. That is how the
                    // Peragus hangar ramp lost its bend boundary: node 7
                    // absorbed into the west leg (root 11 -> 7), and node 17
                    // — which corner-fold had just claimed for that same
                    // west leg — read sizeByRoot[7] == 1, passed the guard
                    // it should have failed, and bridged into the east leg,
                    // welding a 2-corridor switchback into one horseshoe
                    // cluster whose centroid sits inside the rock between
                    // the legs.
                    int mergedSize = sizeByRoot[UFFind(x)] +
                                     sizeByRoot[UFFind(coreNbA)];
                    UFUnite(x, coreNbA);
                    int newRoot = UFFind(x);
                    if (newRoot >= 0 && newRoot < n)
                        sizeByRoot[newRoot] = mergedSize;
                    ++absorbAdj;
                    changed = true;
                }
            }
        }
    }
    acclog::Write(
        "WallTopo",
        "  absorb [adj]: absorbed=%d vetoedByDoor=%d vetoedByWall=%d "
        "(cap=%.0fm)",
        absorbAdj, absorbAdjVetoDoor, absorbAdjVetoWall, kStragglerAbsorbMaxM);
    bp.absorbAdj = absorbAdj;
}

// (b) Bounding-box admission for remaining graph-unattached singletons.
// Snapshot taken now, after the adjacency cascade, so the bbox reflects
// the final cores. Single pass over a frozen snapshot — absorbing a
// candidate does not recompute the bbox or re-test others (iteration
// could chain-absorb a real corridor sticking out of a Platz).
void AbsorbStragglersByBoundingBox(void* area,
                                   const acc::engine::navgraph::NavGraphSnapshot& g,
                                   int n, BuildPasses& bp) {
    const float* nodeFloorZ = bp.nodeFloorZ;
    const acc::engine::WallEdge* gw = bp.gw;
    const int  gwCount = bp.gwCount;
    const bool haveGW  = bp.haveGW;

    int bboxSnapRoot[kMaxNodes];
    int bboxSnapSize[kMaxNodes];
    for (int i = 0; i < n; ++i) {
        bboxSnapRoot[i] = UFFind(i);
        bboxSnapSize[i] = 0;
    }
    for (int i = 0; i < n; ++i) {
        int r = bboxSnapRoot[i];
        if (r >= 0 && r < n) ++bboxSnapSize[r];
    }

    struct ClusterBbox {
        float minX, maxX, minY, maxY;
        bool  valid;
    };
    ClusterBbox bboxByRoot[kMaxNodes];
    for (int i = 0; i < n; ++i) bboxByRoot[i].valid = false;
    for (int m = 0; m < n; ++m) {
        int root = bboxSnapRoot[m];
        if (root < 0 || root >= n) continue;
        if (bboxSnapSize[root] < 2) continue;
        ClusterBbox& bb = bboxByRoot[root];
        const Vector p = g.nodes[m].pos;
        if (!bb.valid) {
            bb.minX = bb.maxX = p.x;
            bb.minY = bb.maxY = p.y;
            bb.valid = true;
        } else {
            if (p.x < bb.minX) bb.minX = p.x;
            if (p.x > bb.maxX) bb.maxX = p.x;
            if (p.y < bb.minY) bb.minY = p.y;
            if (p.y > bb.maxY) bb.maxY = p.y;
        }
    }

    int bboxAbsorbed     = 0;
    int bboxVetoedByWall = 0;
    int bboxVetoedByDoor = 0;
    int bboxNoZMatch     = 0;
    int bboxAmbiguous    = 0;
    for (int x = 0; x < n; ++x) {
        if (bboxSnapSize[bboxSnapRoot[x]] != 1) continue;  // singletons only
        const Vector px = g.nodes[x].pos;

        int   bestRoot   = -1;
        int   bestMember = -1;
        float bestD2     = 1e30f;
        int   containing = 0;
        for (int root = 0; root < n; ++root) {
            const ClusterBbox& bb = bboxByRoot[root];
            if (!bb.valid) continue;
            if (px.x < bb.minX || px.x > bb.maxX) continue;
            if (px.y < bb.minY || px.y > bb.maxY) continue;

            int   nearestMember = -1;
            float nearestD2     = 1e30f;
            for (int m = 0; m < n; ++m) {
                if (bboxSnapRoot[m] != root) continue;
                float dz = g.nodes[m].pos.z - px.z;
                if (std::fabs(dz) > kMergeMaxZM) continue;
                float d2 = DistXYSq(g.nodes[m].pos, px);
                if (d2 < nearestD2) {
                    nearestD2     = d2;
                    nearestMember = m;
                }
            }
            if (nearestMember < 0) continue;

            ++containing;
            if (nearestD2 < bestD2) {
                bestD2     = nearestD2;
                bestRoot   = root;
                bestMember = nearestMember;
            }
        }

        if (bestRoot < 0) {
            for (int root = 0; root < n; ++root) {
                const ClusterBbox& bb = bboxByRoot[root];
                if (!bb.valid) continue;
                if (px.x < bb.minX || px.x > bb.maxX) continue;
                if (px.y < bb.minY || px.y > bb.maxY) continue;
                ++bboxNoZMatch;
                acclog::Write(
                    "WallTopo",
                    "  bbox-absorb VETOED (z): node[%d] (%.1f,%.1f,%.1f) "
                    "inside bbox of core=%d but no member within %.1fm in Z "
                    "— multi-floor protection",
                    x, px.x, px.y, px.z, root, kMergeMaxZM);
                break;
            }
            continue;
        }

        if (containing > 1) {
            ++bboxAmbiguous;
            acclog::Write(
                "WallTopo",
                "  bbox-absorb AMBIGUOUS: node[%d] (%.1f,%.1f) sits inside %d "
                "core bboxes; picking nearest (core=%d via member[%d] at %.1fm)",
                x, px.x, px.y, containing,
                bestRoot, bestMember, std::sqrt(bestD2));
        }

        EdgeResult er = ClassifyEdge(area, px, g.nodes[bestMember].pos,
                                     "bbox-absorb");
        if (er.kind == kEdgeBlocked) {
            ++bboxVetoedByWall;
            acclog::Write(
                "WallTopo",
                "  bbox-absorb VETOED (wall): node[%d] (%.1f,%.1f) inside "
                "core=%d bbox but segment to member[%d] (%.1f,%.1f) crosses "
                "a wall",
                x, px.x, px.y, bestRoot,
                bestMember, g.nodes[bestMember].pos.x,
                g.nodes[bestMember].pos.y);
            continue;
        }
        if (er.kind == kEdgeDoor) {
            ++bboxVetoedByDoor;
            acclog::Write(
                "WallTopo",
                "  bbox-absorb VETOED (door): node[%d] (%.1f,%.1f) inside "
                "core=%d bbox but segment to member[%d] (%.1f,%.1f) crosses "
                "door[%d]",
                x, px.x, px.y, bestRoot,
                bestMember, g.nodes[bestMember].pos.x,
                g.nodes[bestMember].pos.y, er.doorIdx);
            continue;
        }

        UFUnite(x, bestMember);
        ++bboxAbsorbed;
        acclog::Write(
            "WallTopo",
            "  bbox-absorb: node[%d] (%.1f,%.1f,%.1f) -> core=%d "
            "(nearest member[%d] at %.1f,%.1f, distance %.1fm)",
            x, px.x, px.y, px.z, bestRoot,
            bestMember, g.nodes[bestMember].pos.x,
            g.nodes[bestMember].pos.y, std::sqrt(bestD2));
    }
    acclog::Write(
        "WallTopo",
        "  bbox-absorb: absorbed=%d vetoedByWall=%d vetoedByDoor=%d "
        "vetoedByZ=%d ambiguous-bbox=%d",
        bboxAbsorbed, bboxVetoedByWall, bboxVetoedByDoor,
        bboxNoZMatch, bboxAmbiguous);
    bp.bboxAbsorbed = bboxAbsorbed;
}

void AbsorbOpenEndedPendants(const acc::engine::navgraph::NavGraphSnapshot& g,
                             int n, BuildPasses& bp) {
    const bool*  nodeDeadEnd = bp.nodeDeadEnd;
    const float* nodeFloorZ  = bp.nodeFloorZ;
    const acc::engine::WallEdge* gw = bp.gw;
    const int  gwCount = bp.gwCount;
    const bool haveGW  = bp.haveGW;
    const float capSq  = kPendantAbsorbMaxM * kPendantAbsorbMaxM;

    int folded = 0, vetoDist = 0, vetoDoor = 0, vetoWall = 0, vetoReal = 0;
    bool changed = true;
    int  guard   = 0;
    while (changed && guard++ < n) {
        changed = false;
        for (int root = 0; root < n && !changed; ++root) {
            if (UFFind(root) != root) continue;

            // Collect this cluster's external neighbours (deduped by the
            // cluster they belong to) and its terminal members.
            int  outRoot = -1;
            int  outNb = -1, outSrc = -1;
            bool multipleOut = false;
            bool anyRealDeadEnd = false, anyTerminal = false;
            for (int m = 0; m < n && !multipleOut; ++m) {
                if (UFFind(m) != root) continue;
                int lo = 0, hi = 0;
                acc::engine::navgraph::NeighbourRange(g, m, lo, hi);
                if (Degree(g, m) == 1) {
                    anyTerminal = true;
                    // A terminal whose dead-end claim the walkmesh
                    // CONFIRMS makes this a genuine region — never fold.
                    if (nodeDeadEnd[m]) anyRealDeadEnd = true;
                }
                for (int e = lo; e < hi; ++e) {
                    int j = static_cast<int>(g.conns[e]);
                    if (j < 0 || j >= n) continue;
                    int jr = UFFind(j);
                    if (jr == root) continue;         // internal edge
                    if (outRoot < 0) {
                        outRoot = jr; outNb = j; outSrc = m;
                    } else if (jr != outRoot) {
                        multipleOut = true;           // not a pendant
                        break;
                    }
                }
            }
            if (multipleOut || outRoot < 0) continue;  // hub, or isolated
            if (!anyTerminal) continue;    // no end to judge
            if (anyRealDeadEnd) { ++vetoReal; continue; }

            // Adjacency guard — see kPendantAbsorbMaxM.
            float dx = g.nodes[outSrc].pos.x - g.nodes[outNb].pos.x;
            float dy = g.nodes[outSrc].pos.y - g.nodes[outNb].pos.y;
            if (dx * dx + dy * dy > capSq) { ++vetoDist; continue; }
            if (FindDoorOnEdge(g.nodes[outSrc].pos,
                               g.nodes[outNb].pos) >= 0) {
                // A door is a perceptual boundary — what is behind it is
                // the next region's business, same rule the other passes use.
                ++vetoDoor;
                continue;
            }
            if (haveGW) {
                Vector a = g.nodes[outSrc].pos, b = g.nodes[outNb].pos;
                float fz = 0.5f * (nodeFloorZ[outSrc] + nodeFloorZ[outNb]);
                a.z = fz; b.z = fz;
                Vector hit{};
                if (acc::engine::SegmentCrossesWalkmesh(gw, gwCount,
                                                        a, b, hit)) {
                    ++vetoWall;
                    continue;
                }
            }

            acclog::Write(
                "WallTopo",
                "  pendant-absorb: cluster=%d (open-ended — terminals "
                "walkmesh-rejected) -> core=%d via member[%d] (%.1f,%.1f) "
                "-> nb[%d] (%.1f,%.1f), %.1fm",
                root, outRoot, outSrc,
                g.nodes[outSrc].pos.x, g.nodes[outSrc].pos.y,
                outNb, g.nodes[outNb].pos.x, g.nodes[outNb].pos.y,
                std::sqrt(dx * dx + dy * dy));
            UFUnite(root, outNb);
            ++folded;
            changed = true;
        }
    }
    acclog::Write("WallTopo",
                  "  pendant-absorb: folded=%d vetoed: realDeadEnd=%d "
                  "distance=%d door=%d wall=%d (cap=%.1fm)",
                  folded, vetoReal, vetoDist, vetoDoor, vetoWall,
                  kPendantAbsorbMaxM);
}

}  // namespace acc::room_topology
