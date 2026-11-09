// The label of a region: decides from the region's shape which kind of
// label it gets (ClassifyClusterShape) and builds it with its exits
// (ClassifyCluster).

#include "room_topology_internal.h"

#include "log.h"

namespace acc::room_topology {

namespace {

void ClassifyClusterShape(const acc::engine::navgraph::NavGraphSnapshot& g,
                     const Vector& centroid,
                     const int* externalNbs,
                     const int* externalSrcs,
                     const int* externalDoorIdx,
                     int externalCount,
                     int trigCount,
                     const std::string& trigEntries,
                     int areaHint, float centroidFloorZ,
                     bool isLargeArea,
                     const float* bboxXY, const float* axisExt,
                     const bool* nodeDeadEnd,
                     bool terminalDeadEndAgrees,
                     std::string& outLabel,
                     int& outKind, int& outSig,
                     bool& outFiltered) {
    using acc::strings::Id;
    outLabel.clear();
    outKind = kKindOpenArea;
    outSig  = kKindOpenArea;
    outFiltered = false;
    int n = static_cast<int>(g.nodes.size());

    if (areaHint != 0) {
        ClassifyAsArea(g, n, centroid, centroidFloorZ, externalNbs,
                       externalSrcs, externalDoorIdx, externalCount,
                       areaHint, isLargeArea, bboxXY, axisExt, nodeDeadEnd,
                       trigEntries, outLabel, outKind, outSig);
        return;
    }

    // externalCount == 0 never reaches here: the caller forces areaHint != 0
    // for any zero-exit cluster (see BuildForArea's isArea test), so an
    // isolated cluster always takes the "Bereich" area path above.
    if (externalCount == 1) {
        ClassifyAsDeadEnd(g, n, centroid, centroidFloorZ,
                          externalNbs, externalDoorIdx,
                          trigCount, terminalDeadEndAgrees, trigEntries,
                          outLabel, outKind, outSig, outFiltered);
        return;
    }

    if (externalCount == 2) {
        ClassifyAsCorridor(g, n, centroid, centroidFloorZ,
                           externalNbs, externalDoorIdx, nodeDeadEnd,
                           trigEntries, outLabel, outKind, outSig);
        return;
    }

    // 3+ external edges = junction or Platz. First pass: per-octant
    // aggregation. octantHasExit[bit] is set whenever any external
    // neighbour bucket-classifies into `bit`. octantAllDeadEnd[bit]
    // starts true and gets cleared the moment a non-dead-end exit
    // appears in that octant — so it stays true only when EVERY exit
    // in the bucket is a degree-1 stub. This fixes the order-dependent
    // first-wins issue where a passable exit could be silently masked
    // by a co-located dead-end (or vice versa).
    bool octantHasExit[8]    = {false, false, false, false,
                                false, false, false, false};
    bool octantAllDeadEnd[8] = {true, true, true, true,
                                true, true, true, true};
    // First-door-per-octant: if any external edge in an octant crosses
    // a CSWSDoor, store its index here. The emit loop swaps the plain
    // direction word for the "Tür DIR" / "Tür DIR nach DEST" rewrite
    // and skips the (Sackgasse) marker for that octant — door wins.
    int octantDoorIdx[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    // Where each octant's exit leads (ExitMark): the first
    // contributing neighbour (a door octant is placed at the door itself,
    // see ExitPoint).
    Vector octantPos[8] = {};
    bool   octantHasPos[8] = {false, false, false, false,
                              false, false, false, false};
    // Ramp aggregation, same conservatism as the area path: annotate an
    // octant only when every contributing real exit crosses the
    // floor-level change (see ExitIsRamp).
    bool octantAnyRamp [8] = {false, false, false, false,
                              false, false, false, false};
    bool octantAnyLevel[8] = {false, false, false, false,
                              false, false, false, false};
    // Per-neighbour classification accumulates into per-octant counters.
    // Per the user's "don't announce nonsense" rule:
    //   - real onward exit (degree ≥ 2)     → octant emits direction
    //   - real dead-end alcove (degree == 1, walkmesh agrees) → emits
    //                                          direction, may carry
    //                                          (Sackgasse) marker
    //   - wall-curve (degree == 1, walkmesh disagrees) → contributes
    //                                          NOTHING — direction is
    //                                          dropped entirely when the
    //                                          octant has no other
    //                                          contributors
    // octantHasExit[bit] is set only by real onward exits or real dead-
    // ends (or doors — see below). octantAllDeadEnd[bit] is cleared by
    // real onward exits, leaving the marker only for octants whose
    // exits are exclusively real dead-end alcoves.
    for (int k = 0; k < externalCount; ++k) {
        int nb = externalNbs[k];
        if (nb < 0 || nb >= n) continue;
        float dx = g.nodes[nb].pos.x - centroid.x;
        float dy = g.nodes[nb].pos.y - centroid.y;
        Id dir = OctantFromVector(dx, dy);
        int bit = OctantBit(dir);
        if (bit < 0) continue;

        int deg = Degree(g, nb);
        // For the alcove test, the "parent" anchoring the rotated probe
        // axis is the cluster member that owns this edge. Falls back to
        // the centroid when externalSrcs is unavailable.
        int src = externalSrcs ? externalSrcs[k] : -1;
        Vector edgeStart = (src >= 0 && src < n)
                               ? g.nodes[src].pos : centroid;

        bool isOnwardExit  = (deg >= 2);
        bool isRealDeadEnd = (deg == 1) && nodeDeadEnd[nb];
        bool isWallCurve   = (deg == 1) && !isRealDeadEnd;

        // Door verdict precomputed during external-edge collection
        // (one ClassifyEdge call per graph edge, walls + doors decided
        // together). Read the cached doorIdx so we don't run a second
        // parallel door test here.
        bool hasDoor = false;
        int precomputedDoor = externalDoorIdx ? externalDoorIdx[k] : -1;
        if (octantDoorIdx[bit] < 0) {
            if (precomputedDoor >= 0) {
                octantDoorIdx[bit] = precomputedDoor;
                hasDoor = true;
                acclog::Write(
                    "WallTopo",
                    "ClassifyCluster: junction at (%.1f,%.1f) octant=%d "
                    "edge from member %d (%.1f,%.1f) -> nb %d (%.1f,%.1f) "
                    "HIT door[%d] at (%.1f,%.1f)",
                    centroid.x, centroid.y, bit,
                    src, edgeStart.x, edgeStart.y,
                    nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y,
                    precomputedDoor,
                    g_graph.doors[precomputedDoor].pos.x,
                    g_graph.doors[precomputedDoor].pos.y);
            }
        } else {
            hasDoor = true;
        }

        // Direction only appears in the label when SOMETHING real
        // exists in this octant. Wall-curves alone don't qualify.
        if (isOnwardExit || isRealDeadEnd || hasDoor) {
            octantHasExit[bit] = true;
            if (!octantHasPos[bit]) {
                octantPos[bit]    = g.nodes[nb].pos;
                octantHasPos[bit] = true;
            }
        }
        if (isOnwardExit || isRealDeadEnd) {
            if (ExitIsRamp(nb, centroidFloorZ)) octantAnyRamp[bit]  = true;
            else                                octantAnyLevel[bit] = true;
        }
        // (Sackgasse) marker survives only when no onward exit shares
        // the octant. Mixed real-dead-end + wall-curve still gets the
        // marker (the real dead-end carries it); mixed real-dead-end +
        // onward exit doesn't (the onward exit dominates).
        if (isOnwardExit) octantAllDeadEnd[bit] = false;

        if (isWallCurve && !hasDoor) {
            acclog::Write(
                "WallTopo",
                "ClassifyCluster: junction at (%.1f,%.1f) octant=%d "
                "neighbour %d (%.1f,%.1f) FAILED rotated walkmesh gate "
                "(parent=%d at %.1f,%.1f) — not contributing to octant",
                centroid.x, centroid.y, bit,
                nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y,
                src, edgeStart.x, edgeStart.y);
        }
    }

    // Demote gate: count octants the player can actually walk into
    // (onward neighbours + doors). Pure-dead-end stubs and wall-curve
    // octants are annotations, not choices. If the effective exit count
    // drops below 3 (octant collisions or walkmesh gates having eaten
    // the rest), the cluster isn't a junction — it's a corridor or
    // dead-end. Demote so the spoken type matches the decision space
    // and drop dead-end stubs from the demoted label (they're noise in
    // a small space — what the player flagged as overdescription in
    // the loot rooms behind security doors).
    bool octantAllRamp[8];
    for (int bit = 0; bit < 8; ++bit) {
        octantAllRamp[bit] = octantAnyRamp[bit] && !octantAnyLevel[bit];
        if (octantAllRamp[bit] && octantHasExit[bit]) {
            acclog::Write("WallTopo",
                          "ClassifyCluster: junction at (%.1f,%.1f) "
                          "octant=%d exit crosses floor-level change — "
                          "ramp annotation",
                          centroid.x, centroid.y, bit);
        }
    }

    int realExitCount = 0;
    int realExitBits[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    for (int bit = 0; bit < 8; ++bit) {
        bool hasOnward = octantHasExit[bit] && !octantAllDeadEnd[bit];
        bool hasDoor   = octantDoorIdx[bit] >= 0;
        if (!hasOnward && !hasDoor) continue;
        realExitBits[realExitCount++] = bit;
    }

    if (realExitCount == 1) {
        ClassifyDemotedToDeadEnd(centroid, octantDoorIdx, realExitBits,
                                 octantAllRamp, octantPos,
                                 trigEntries, outLabel, outKind, outSig);
        return;
    }

    if (realExitCount == 2) {
        ClassifyDemotedToCorridor(centroid, octantDoorIdx, realExitBits,
                                  octantAllRamp, octantPos, trigEntries,
                                  outLabel, outKind, outSig);
        return;
    }

    EmitJunctionLabel(octantHasExit, octantAllDeadEnd, octantDoorIdx,
                      octantAllRamp, octantPos, trigEntries,
                      outLabel, outKind, outSig);
}

}  // namespace

// Classification entry point: pick the shape, then guarantee the cluster
// ends up with SOME label.
//
// An empty label is not a quiet failure — LookupAt skips unlabelled nodes
// outright, so the whole cluster becomes invisible and the player hears
// the neutral area fallback while standing in a region we classified
// perfectly well. The shape branches can legitimately produce nothing:
// RenderCorridorAxis declines when both ends of a run bucket into the
// SAME octant (there is no axis to name), which is what silenced the
// merged 17-node Ebon-Hawk comb — both its exits bear south from the
// centroid. The area path needs no axis, so it is the natural fallback.
void ClassifyCluster(const acc::engine::navgraph::NavGraphSnapshot& g,
                     const Vector& centroid,
                     const int* externalNbs,
                     const int* externalSrcs,
                     const int* externalDoorIdx,
                     int externalCount,
                     const int* trigIdxs, int trigCount,
                     int areaHint, float centroidFloorZ,
                     bool isLargeArea,
                     const float* bboxXY, const float* axisExt,
                     const bool* nodeDeadEnd,
                     bool terminalDeadEndAgrees,
                     std::string& outLabel,
                     int& outKind, int& outSig,
                     bool& outFiltered) {
    // Attached transition-trigger exits, rendered once and shared by
    // every path below.
    std::string trigEntries;
    for (int t = 0; t < trigCount; ++t) {
        std::string entry = RenderTransitionExit(trigIdxs[t], centroid);
        AppendListEntry(trigEntries, entry);
        if (trigIdxs[t] >= 0 && trigIdxs[t] < g_graph.transition_count) {
            NoteExit(entry, g_graph.transitions[trigIdxs[t]].pos);
        }
    }
    const size_t trigMarks = g_exit_sink ? g_exit_sink->size() : 0;

    ClassifyClusterShape(g, centroid, externalNbs, externalSrcs,
                         externalDoorIdx, externalCount,
                         trigCount, trigEntries,
                         areaHint, centroidFloorZ, isLargeArea, bboxXY,
                         axisExt, nodeDeadEnd, terminalDeadEndAgrees,
                         outLabel, outKind, outSig, outFiltered);
    if (!outLabel.empty()) return;

    // The shape branch gave up; whatever exits it noted belong to a label
    // that was never written. The area path below notes its own.
    if (g_exit_sink) g_exit_sink->resize(trigMarks);
    int n = static_cast<int>(g.nodes.size());
    ClassifyAsArea(g, n, centroid, centroidFloorZ, externalNbs,
                   externalSrcs, externalDoorIdx, externalCount,
                   /*areaHint=*/2, isLargeArea, bboxXY, axisExt,
                   nodeDeadEnd, trigEntries, outLabel, outKind, outSig);
    outFiltered = false;
    acclog::Write(
        "WallTopo",
        "ClassifyCluster: shape branch produced no label at (%.1f,%.1f) "
        "(exits=%d) — an unlabelled cluster is invisible to LookupAt; "
        "rendered through the area path instead → \"%s\"",
        centroid.x, centroid.y, externalCount, outLabel.c_str());
}

}  // namespace acc::room_topology
