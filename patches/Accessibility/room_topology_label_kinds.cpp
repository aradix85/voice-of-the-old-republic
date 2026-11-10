// The label of one region per kind: open area or room, dead end, corridor,
// the two demotions (to dead end, to corridor) and the junction label.

#include "room_topology_internal.h"

#include "log.h"
#include "strfmt.h"             // Format — heap-backed printf, never truncates announcements

namespace acc::room_topology {

namespace {

// Order in which we emit direction words at junctions / plazas. N
// first then clockwise (NE, E, SE, S, SW, W, NW) — feels natural for
// a player who orients on a compass and reads exits clockwise.
constexpr int kOctantEmitOrder[8] = {
    2, 1, 0, 7, 6, 5, 4, 3
};

// Is this corridor end a cul-de-sac worth flagging? True when the end
// neighbour has no further graph edges, carries no area-transition
// trigger (the ClassifyAsDeadEnd exemption — the walkmesh ends because
// the AREA ends), and the walkmesh confirms the deadness: either the
// end forms a genuine alcove (same gate junctions use) or a probe cast
// onward past the end hits a wall within a few metres. The probe is
// what a bare degree test can't provide — a degree-1 patrol anchor
// sitting in CONTINUING open space (wall-curve artefact) probes long
// and stays unmarked, so the marker can never hide a real way onward.
bool CorridorEndIsDead(const acc::engine::navgraph::NavGraphSnapshot& g,
                       int nb, const Vector& centroid,
                       const bool* nodeDeadEnd) {
    if (Degree(g, nb) >= 2) return false;
    for (int t = 0; t < g_graph.transition_count; ++t) {
        if (g_graph.transitions[t].nearestNode == nb) return false;
    }
    if (nodeDeadEnd[nb]) return true;
    // Beyond-probe: continue the corridor axis past the end node. A
    // terminus wall sits close (mine shafts droid pocket: 3.3m); a
    // continuing space probes past the threshold. -1 (no wall cache)
    // leaves the end unmarked — a wrong "dead end" on the only onward
    // arm is the inverse of the bug this fixes, so fail toward silence.
    const float kBeyondWallMaxM = 6.0f;
    float bx = g.nodes[nb].pos.x - centroid.x;
    float by = g.nodes[nb].pos.y - centroid.y;
    float d = acc::wall_probe::ProbeDistance(g.nodes[nb].pos, bx, by);
    return d >= 0.0f && d <= kBeyondWallMaxM;
}

}  // namespace

namespace {

// The exits of an area, bucketed by compass octant.
struct AreaOctants {
    bool   has[8]  = {};
    int    door[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    Vector pos[8]  = {};
    // An octant is announced as a ramp only when EVERY real exit in it
    // crosses the level change: one same-floor exit and the plain direction
    // wins, since "ramp" would claim a climb the player can avoid.
    bool   anyRamp[8]  = {};
    bool   anyLevel[8] = {};
};

AreaOctants CollectAreaExits(const acc::engine::navgraph::NavGraphSnapshot& g, int n,
                             const Vector& centroid, float centroidFloorZ,
                             const int* externalNbs, const int* externalSrcs,
                             const int* externalDoorIdx, int externalCount,
                             const bool* nodeDeadEnd) {
    AreaOctants oct;
    for (int k = 0; k < externalCount; ++k) {
        int nb = externalNbs[k];
        if (nb < 0 || nb >= n) continue;
        int bit = OctantBit(OctantFromVector(g.nodes[nb].pos.x - centroid.x,
                                             g.nodes[nb].pos.y - centroid.y));
        if (bit < 0) continue;
        // The anchor of an exit is the IN-ZONE end of its edge (the member
        // the player must reach to leave), not the outside neighbour:
        // boundary edges run long diagonals, and a neighbour can sit
        // mid-zone while its member is in a far corner. Without a member
        // the centroid stands in, which keeps the far qualifier silent.
        int src = externalSrcs ? externalSrcs[k] : -1;
        Vector edgeStart = (src >= 0 && src < n) ? g.nodes[src].pos : centroid;
        int door = externalDoorIdx ? externalDoorIdx[k] : -1;
        if (door >= 0) {
            if (oct.door[bit] < 0) oct.door[bit] = door;
            if (!oct.has[bit]) oct.pos[bit] = edgeStart;
            oct.has[bit] = true;
            continue;
        }
        // A degree-1 neighbour is a wall-curve artefact unless the walkmesh
        // confirms a dead end there.
        int deg = Degree(g, nb);
        bool realExit = (deg >= 2) || (deg == 1 && nodeDeadEnd[nb]);
        if (realExit) {
            if (!oct.has[bit]) oct.pos[bit] = edgeStart;
            oct.has[bit] = true;
            if (ExitIsRamp(nb, centroidFloorZ)) oct.anyRamp[bit]  = true;
            else                                oct.anyLevel[bit] = true;
        }
    }
    return oct;
}

struct AreaExitList {
    std::string text;
    int  mask = 0;         // octants with an exit
    bool anyFar = false;   // some entry carries a far-end qualifier
    bool anyRamp = false;  // some entry is announced as a ramp
};

// The exit list, clockwise from north, and the exit points for the N key.
AreaExitList RenderAreaExits(const AreaOctants& oct, const float* bboxXY) {
    using acc::strings::Id;
    AreaExitList out;
    for (int idx = 0; idx < 8; ++idx) {
        int bit = kOctantEmitOrder[idx];
        if (!oct.has[bit]) continue;
        out.mask |= (1 << bit);
        Id dirId = BitToOctant(bit);
        const char* dirWord = acc::strings::Get(dirId);
        std::string entry;
        if (oct.door[bit] >= 0 && dirWord[0]) {
            entry = RenderDoorDirection(oct.door[bit], dirWord);
        } else {
            bool markRamp = oct.anyRamp[bit] && !oct.anyLevel[bit];
            entry = DirEntry(dirId, /*markDeadEnd=*/false, markRamp);
            if (markRamp) {
                out.anyRamp = true;
                acclog::Write("WallTopo",
                              "ClassifyAsArea: octant=%d exit crosses floor-level "
                              "change - rendered as \"%s\"", bit, entry.c_str());
            }
        }
        // Joined with a space, never a comma: a comma inside an entry would
        // read as one more exit in the list.
        const char* farSfx = FarEndSuffix(bit, oct.pos[bit], bboxXY);
        if (farSfx && !entry.empty()) {
            entry += ' ';
            entry += farSfx;
            out.anyFar = true;
        }
        AppendListEntry(out.text, entry);
        NoteExit(entry, ExitPoint(oct.door[bit], oct.pos[bit]));
    }
    return out;
}

// True when the exits are exactly the two plain ends of the elongation
// axis: "Area East-West" then says it all. A door exit is always kept (its
// destination is information the axis word does not carry), and so is a
// far-end qualifier or a ramp.
bool AxisCoversExits(acc::strings::Id axisId, const AreaExitList& exits,
                     const AreaOctants& oct) {
    if (exits.anyFar || exits.anyRamp) return false;
    const int axisMask = AxisOctantMask(axisId);
    if (axisMask == 0 || exits.mask != axisMask) return false;
    for (int bit = 0; bit < 8; ++bit) {
        if ((axisMask & (1 << bit)) && oct.door[bit] >= 0) return false;
    }
    return true;
}

}  // namespace

// Area path: probe-owned merged spaces (open / room / merged big
// junction). One neutral "Area" label, long-axis cue only when
// elongated, then the exit list — door rewrites kept (the navigational
// anchors), wall-curve degree-1 artefacts dropped.
void ClassifyAsArea(const acc::engine::navgraph::NavGraphSnapshot& g, int n,
                    const Vector& centroid, float centroidFloorZ,
                    const int* externalNbs, const int* externalSrcs,
                    const int* externalDoorIdx, int externalCount,
                    int areaHint, bool isLargeArea,
                    const float* bboxXY, const float* axisExt,
                    const bool* nodeDeadEnd,
                    const std::string& trigEntries,
                    std::string& outLabel, int& outKind, int& outSig) {
    using acc::strings::Id;
    const size_t marksAtEntry = g_exit_sink ? g_exit_sink->size() : 0;
    const AreaOctants oct = CollectAreaExits(g, n, centroid, centroidFloorZ, externalNbs,
                                             externalSrcs, externalDoorIdx,
                                             externalCount, nodeDeadEnd);
    const AreaExitList exits = RenderAreaExits(oct, bboxXY);
    // Attached transition triggers are exits like any other — appended
    // last so the compass entries keep their canonical clockwise order.
    std::string dirList = exits.text;
    AppendListEntry(dirList, trigEntries);

    Id axisId = Id::AxisEastWest;
    const bool elong = ComputeCentroidAxis(centroid, centroidFloorZ, axisExt, axisId);
    // "Large area" sets expectations only; axis, exits and kind are the same.
    const char* noun  = acc::strings::Get(isLargeArea ? Id::AreaNounLarge : Id::AreaNoun);
    const char* axisW = elong ? acc::strings::Get(axisId) : "";

    const bool axisCoversExits =
        elong && trigEntries.empty() && AxisCoversExits(axisId, exits, oct);
    const bool haveExits = !dirList.empty() && !axisCoversExits;
    // No exit list in the label: the axis word alone is spoken, centred,
    // and the exits it covers stay as unspoken points for the N key.
    if (axisCoversExits && g_exit_sink) {
        for (size_t i = marksAtEntry; i < g_exit_sink->size(); ++i) {
            (*g_exit_sink)[i].text.clear();
        }
    }

    if (axisW[0] && haveExits) {
        outLabel = acc::strfmt::Format(acc::strings::Get(Id::FmtAreaAxisExits), noun,
                                       axisW, dirList.c_str());
    } else if (haveExits) {
        outLabel = acc::strfmt::Format(acc::strings::Get(Id::FmtAreaExits), noun,
                                       dirList.c_str());
    } else if (axisW[0]) {
        outLabel = acc::strfmt::Format(acc::strings::Get(Id::FmtAreaAxisOnly), noun,
                                       axisW);
    } else {
        outLabel = noun;
    }
    outKind = (areaHint == 1) ? kKindRoom : kKindPlaza;
    outSig  = (outKind & 0xff) | ((exits.mask & 0xff) << 8);
}

// Exactly one external edge — a dead end. Names what is at the end of
// it (door, trigger, or nothing) so the player knows before walking in.
void ClassifyAsDeadEnd(const acc::engine::navgraph::NavGraphSnapshot& g, int n,
                       const Vector& centroid, float centroidFloorZ,
                       const int* externalNbs,
                       const int* externalDoorIdx, int trigCount,
                       bool gateAgrees,
                       const std::string& trigEntries,
                       std::string& outLabel, int& outKind, int& outSig,
                       bool& outFiltered) {
    using acc::strings::Id;
    int nb = externalNbs[0];
    if (nb < 0 || nb >= n) return;

    // A "dead end" carrying a transition trigger is the area exit,
    // not a dead end — the walkmesh stops because the AREA stops.
    // Render as a passage: the transition entry first (the salient
    // information), then the direction back into the map. Corridor
    // kind so the announce path treats it as a normal passage.
    if (!trigEntries.empty()) {
        float bdx = g.nodes[nb].pos.x - centroid.x;
        float bdy = g.nodes[nb].pos.y - centroid.y;
        const char* backWord =
            acc::strings::Get(OctantFromVector(bdx, bdy));
        std::string list = trigEntries;
        if (backWord && backWord[0]) {
            AppendListEntry(list, std::string(backWord));
            NoteExit(backWord, g.nodes[nb].pos);
        }
        outLabel = list;
        outKind  = kKindCorridor;
        int backBit = OctantBit(OctantFromVector(bdx, bdy));
        outSig = (kKindCorridor & 0xff) |
                 (((backBit < 0 ? 0 : backBit) & 0xff) << 8);
        acclog::Write(
            "WallTopo",
            "ClassifyCluster: degree-1 at (%.1f,%.1f) carries %d "
            "transition trigger%s — rendered as passage \"%s\" instead "
            "of dead end",
            centroid.x, centroid.y, trigCount,
            trigCount == 1 ? "" : "s", outLabel.c_str());
        return;
    }

    // Walkmesh-shape verdict, precomputed per node with the other
    // node-local shape facts (ComputeNodeShapeFeatures) and handed in
    // by the caller, which resolves it from this cluster's TERMINAL
    // member. It used to be re-probed here from the cluster CENTROID —
    // mid-run for any multi-node cluster, so the "wall close behind"
    // test could never pass and every dead-end run longer than a few
    // metres was wrongly filtered.
    //
    // A rejected verdict now normally un-clusters the region entirely
    // (AbsorbOpenEndedPendants folds it into the space it hangs off),
    // so what still reaches this branch is a pendant that could not
    // fold: too far from its neighbour, or separated by a door or a
    // wall. There the historical treatment stands unchanged — render
    // the label but flag it rescue-only, so LookupAt speaks it just
    // when no unfiltered candidate is within snap range. That is what
    // keeps the documented Bucket-3 alcove (tar_m11aa: rejected node
    // 0.8m away, nearest labelled cluster 23.8m) audible instead of
    // falling through to the neutral "Area".
    if (!gateAgrees) {
        outFiltered = true;
        acclog::Write(
            "WallTopo",
            "ClassifyCluster: degree-1 at (%.1f,%.1f,%.1f) FAILED "
            "walkmesh-shape gate (parent=%d at %.1f,%.1f) — labelling "
            "as filtered dead end (LookupAt rescue only)",
            centroid.x, centroid.y, centroid.z,
            nb, g.nodes[nb].pos.x, g.nodes[nb].pos.y);
    }
    float dx = g.nodes[nb].pos.x - centroid.x;
    float dy = g.nodes[nb].pos.y - centroid.y;
    Id dir = OctantFromVector(dx, dy);
    const char* word = acc::strings::Get(dir);

    // Door verdict was precomputed during external-edge collection
    // (ClassifyEdge ran the wall + door tests once per real graph
    // edge). Read the cached doorIdx rather than re-querying — that
    // keeps walls + doors decided by the same primitive instead of
    // running parallel checks here.
    int doorIdx = externalDoorIdx ? externalDoorIdx[0] : -1;
    if (doorIdx >= 0 && word && word[0]) {
        outLabel = RenderDoorDirection(doorIdx, word);
        NoteExit(outLabel, ExitPoint(doorIdx, g.nodes[nb].pos));
        if (gateAgrees) {
            acclog::Write(
                "WallTopo",
                "ClassifyCluster: degree-1 at (%.1f,%.1f,%.1f) HIT door "
                "idx=%d on edge → \"%s\"",
                centroid.x, centroid.y, centroid.z, doorIdx,
                outLabel.c_str());
        }
    } else {
        // The single direction is the way OUT — if that way out climbs
        // a level, say so ("dead end, Ramp North").
        std::string w = DirEntry(dir, /*markDeadEnd=*/false,
                                 ExitIsRamp(nb, centroidFloorZ));
        const char* fmt  = acc::strings::Get(Id::FmtMapCursorDeadEnd);
        if (fmt[0] && !w.empty()) {
            outLabel = acc::strfmt::Format(fmt, w.c_str());
            NoteExit(w, g.nodes[nb].pos);  // the way out of the dead end
        }
    }
    outKind = kKindDeadEnd;
    int octBit = OctantBit(dir);
    outSig = (kKindDeadEnd & 0xff) | ((octBit & 0xff) << 8);
    return;
}

// Exactly two external edges — a corridor run, announced by the axis
// its two ends lie on.
void ClassifyAsCorridor(const acc::engine::navgraph::NavGraphSnapshot& g, int n,
                        const Vector& centroid, float centroidFloorZ,
                        const int* externalNbs,
                        const int* externalDoorIdx,
                        const bool* nodeDeadEnd,
                        const std::string& trigEntries,
                        std::string& outLabel, int& outKind, int& outSig) {
    using acc::strings::Id;
    int nbA = externalNbs[0];
    int nbB = externalNbs[1];
    if (nbA < 0 || nbA >= n || nbB < 0 || nbB >= n) return;
    // Octant per endpoint, relative to the centroid (cluster
    // member's position). RenderCorridorAxis picks the right
    // form — single axis word for symmetric corridors, two-end
    // form for L-shaped / asymmetric corridors.
    int bitA = OctantBit(OctantFromVector(g.nodes[nbA].pos.x - centroid.x,
                                          g.nodes[nbA].pos.y - centroid.y));
    int bitB = OctantBit(OctantFromVector(g.nodes[nbB].pos.x - centroid.x,
                                          g.nodes[nbB].pos.y - centroid.y));
    // Door verdicts precomputed on the actual graph edges, kept PER END
    // (member→nbA and member→nbB are different exits and routinely
    // different doors). Collapsing them onto one index is what made the
    // Peragus hangar ramp announce "Door South-East, South-West to
    // Treibstoffdepot": the noun belonged to the south-west end but was
    // spoken against the south-east one, and the south-east end's own
    // "security door" was never voiced at all.
    int doorA = externalDoorIdx ? externalDoorIdx[0] : -1;
    int doorB = externalDoorIdx ? externalDoorIdx[1] : -1;
    // Neither graph edge carried a door: fall back to the corridor-span
    // test for a door that sits off both edges but still on the line
    // between the ends, and attribute it to the end it actually stands
    // at rather than smearing it across both.
    if (doorA < 0 && doorB < 0) {
        int spanDoor = FindDoorOnEdge(g.nodes[nbA].pos, g.nodes[nbB].pos);
        if (spanDoor >= 0 && spanDoor < g_graph.door_count) {
            const Vector& dp = g_graph.doors[spanDoor].pos;
            float ax = dp.x - g.nodes[nbA].pos.x;
            float ay = dp.y - g.nodes[nbA].pos.y;
            float bx = dp.x - g.nodes[nbB].pos.x;
            float by = dp.y - g.nodes[nbB].pos.y;
            if (ax * ax + ay * ay <= bx * bx + by * by) doorA = spanDoor;
            else                                        doorB = spanDoor;
        }
    }
    // Dead-end marker per end, door wins (a named gateway is the
    // actionable information; what's behind it is the next cluster's
    // business). Evaluated after the span-door fallback so a door
    // found on the span still suppresses the marker for its end.
    bool deadA = (doorA < 0) && CorridorEndIsDead(g, nbA, centroid, nodeDeadEnd);
    bool deadB = (doorB < 0) && CorridorEndIsDead(g, nbB, centroid, nodeDeadEnd);
    // Ramp marker per end, same precedence as elsewhere (door > dead >
    // ramp): which end of the run climbs to another level is the cue
    // the port-side field was missing — its north-east end IS the way
    // up to the spine, but nothing said so.
    bool rampA = (doorA < 0) && ExitIsRamp(nbA, centroidFloorZ);
    bool rampB = (doorB < 0) && ExitIsRamp(nbB, centroidFloorZ);
    outLabel = RenderCorridorAxis(bitA, doorA, deadA, bitB, doorB, deadB,
                                  rampA, rampB,
                                  ExitPoint(doorA, g.nodes[nbA].pos),
                                  ExitPoint(doorB, g.nodes[nbB].pos));
    AppendListEntry(outLabel, trigEntries);
    if (doorA >= 0 || doorB >= 0 || deadA || deadB || rampA || rampB) {
        acclog::Write(
            "WallTopo",
            "ClassifyCluster: degree-2 corridor at (%.1f,%.1f,%.1f) "
            "HIT door A=%d B=%d dead A=%d B=%d ramp A=%d B=%d "
            "on segment → \"%s\"",
            centroid.x, centroid.y, centroid.z, doorA, doorB,
            deadA ? 1 : 0, deadB ? 1 : 0,
            rampA ? 1 : 0, rampB ? 1 : 0, outLabel.c_str());
    }
    outKind = kKindCorridor;
    bool loIsA = (bitA < bitB);
    int deadMask = ((loIsA ? deadA : deadB) ? 1 : 0) |
                   ((loIsA ? deadB : deadA) ? 2 : 0);
    outSig = (kKindCorridor & 0xff) |
             (((loIsA ? bitA : bitB) & 0xff) << 8) |
             (((loIsA ? bitB : bitA) & 0xff) << 16) |
             (deadMask << 24);
    return;
}

// 3+ external edges that the demote gate reduced to ONE walkable
// octant: speak it as a dead end, not a junction.
void ClassifyDemotedToDeadEnd(const Vector& centroid,
                              const int octantDoorIdx[8],
                              const int realExitBits[8],
                              const bool octantAllRamp[8],
                              const Vector octantPos[8],
                              const std::string& trigEntries,
                              std::string& outLabel, int& outKind,
                              int& outSig) {
    using acc::strings::Id;
    int bit = realExitBits[0];
    Id dirId = BitToOctant(bit);
    const char* dirWord = acc::strings::Get(dirId);
    if (!trigEntries.empty()) {
        // Same rule as the degree-1 path: a transition trigger makes
        // this a passage toward the exit, not a dead end.
        std::string list = trigEntries;
        std::string way;
        if (octantDoorIdx[bit] >= 0) {
            way = RenderDoorDirection(octantDoorIdx[bit],
                                      dirWord ? dirWord : "");
        } else if (dirWord && dirWord[0]) {
            way = dirWord;
        }
        AppendListEntry(list, way);
        NoteExit(way, ExitPoint(octantDoorIdx[bit], octantPos[bit]));
        outLabel = list;
        outKind  = kKindCorridor;
        outSig   = (kKindCorridor & 0xff) | ((bit & 0xff) << 8);
        acclog::Write(
            "WallTopo",
            "ClassifyCluster: junction at (%.1f,%.1f) carries transition "
            "trigger — rendered as passage \"%s\" instead of dead end",
            centroid.x, centroid.y, outLabel.c_str());
        return;
    }
    if (octantDoorIdx[bit] >= 0) {
        outLabel = RenderDoorDirection(octantDoorIdx[bit],
                                       dirWord ? dirWord : "");
        NoteExit(outLabel, ExitPoint(octantDoorIdx[bit], octantPos[bit]));
    } else {
        std::string w = DirEntry(dirId, /*markDeadEnd=*/false,
                                 octantAllRamp[bit]);
        const char* fmt = acc::strings::Get(Id::FmtMapCursorDeadEnd);
        if (fmt[0] && !w.empty()) {
            outLabel = acc::strfmt::Format(fmt, w.c_str());
            NoteExit(w, octantPos[bit]);  // the way out of the dead end
        }
    }
    outKind = kKindDeadEnd;
    outSig  = (kKindDeadEnd & 0xff) | ((bit & 0xff) << 8);
    acclog::Write(
        "WallTopo",
        "ClassifyCluster: junction at (%.1f,%.1f) DEMOTED to dead end "
        "(real-exits=1) → \"%s\"",
        centroid.x, centroid.y, outLabel.c_str());
    return;
}

// Same gate, TWO walkable octants: speak it as a corridor.
void ClassifyDemotedToCorridor(const Vector& centroid,
                               const int octantDoorIdx[8],
                               const int realExitBits[8],
                               const bool octantAllRamp[8],
                               const Vector octantPos[8],
                               const std::string& trigEntries,
                               std::string& outLabel, int& outKind,
                               int& outSig) {
    using acc::strings::Id;
    int bitA = realExitBits[0];
    int bitB = realExitBits[1];
    // octantDoorIdx is already per-octant here — index it by each end's
    // own octant instead of collapsing onto the first door found. No
    // dead-end marks: the demote gate only kept octants with an onward
    // exit or a door, so neither surviving end is a cul-de-sac.
    int doorA = (bitA >= 0 && bitA < 8) ? octantDoorIdx[bitA] : -1;
    int doorB = (bitB >= 0 && bitB < 8) ? octantDoorIdx[bitB] : -1;
    bool rampA = (doorA < 0) && bitA >= 0 && bitA < 8 && octantAllRamp[bitA];
    bool rampB = (doorB < 0) && bitB >= 0 && bitB < 8 && octantAllRamp[bitB];
    outLabel = RenderCorridorAxis(bitA, doorA, /*deadA=*/false,
                                  bitB, doorB, /*deadB=*/false,
                                  rampA, rampB,
                                  ExitPoint(doorA, octantPos[bitA]),
                                  ExitPoint(doorB, octantPos[bitB]));
    AppendListEntry(outLabel, trigEntries);
    outKind = kKindCorridor;
    int sigBitLo = (bitA < bitB) ? bitA : bitB;
    int sigBitHi = (bitA < bitB) ? bitB : bitA;
    outSig = (kKindCorridor & 0xff) |
             ((sigBitLo & 0xff) << 8) |
             ((sigBitHi & 0xff) << 16);
    acclog::Write(
        "WallTopo",
        "ClassifyCluster: junction at (%.1f,%.1f) DEMOTED to Korridor "
        "(real-exits=2, octants=%d+%d) → \"%s\"",
        centroid.x, centroid.y, bitA, bitB, outLabel.c_str());
    return;
}

// Second pass: emit octants in canonical order (N, NE, E, SE, S,
// SW, W, NW). Stable across runs; matches the way a compass-oriented
// player scans for exits clockwise.
void EmitJunctionLabel(const bool octantHasExit[8],
                       const bool octantAllDeadEnd[8],
                       const int octantDoorIdx[8],
                       const bool octantAllRamp[8],
                       const Vector octantPos[8],
                       const std::string& trigEntries,
                       std::string& outLabel, int& outKind, int& outSig) {
    using acc::strings::Id;
    std::string dirList;
    int mask = 0;
    int deadEndMask = 0;
    for (int idx = 0; idx < 8; ++idx) {
        int bit = kOctantEmitOrder[idx];
        if (!octantHasExit[bit]) continue;
        mask |= (1 << bit);
        Id dirId = BitToOctant(bit);

        // Door wins over dead end: when an octant carries a door, the
        // (dead end) annotation is suppressed and the direction word
        // is replaced with the "Door DIR" form. The door is the player-
        // actionable information; dead end is metadata about geometry
        // beyond the door that doesn't matter once we've named the
        // gateway.
        if (octantDoorIdx[bit] >= 0) {
            const char* dirWord = acc::strings::Get(dirId);
            if (dirWord[0]) {
                std::string entry = RenderDoorDirection(octantDoorIdx[bit], dirWord);
                AppendListEntry(dirList, entry);
                NoteExit(entry, ExitPoint(octantDoorIdx[bit], octantPos[bit]));
            }
            continue;
        }

        bool markDeadEnd = octantAllDeadEnd[bit];
        if (markDeadEnd) deadEndMask |= (1 << bit);
        // Priority per annotation: door > dead-end > ramp > plain.
        std::string entry = DirEntry(dirId, markDeadEnd,
                                     !markDeadEnd && octantAllRamp[bit]);
        AppendListEntry(dirList, entry);
        NoteExit(entry, octantPos[bit]);
}

// Attached transition triggers appended last, same as the area path.
AppendListEntry(dirList, trigEntries);

// Reached by a hub (singleton or merged) with no open/room geometry, so
// this path renders "junction". A hub that IS open/room was flagged a
// probe-owned place by the caller and took the "Area" area path above.
const char* fmt = acc::strings::Get(Id::FmtMapCursorJunctionDirs);
if (fmt[0] && !dirList.empty()) {
    outLabel = acc::strfmt::Format(fmt, dirList.c_str());
} else {
    const char* bare = acc::strings::Get(Id::MapCursorJunction);
    if (bare[0]) {
        outLabel = bare;
    }
}
outKind = kKindJunction;
outSig  = (kKindJunction & 0xff) |
          ((mask & 0xff) << 8) |
          ((deadEndMask & 0xff) << 16);
}

}  // namespace acc::room_topology
