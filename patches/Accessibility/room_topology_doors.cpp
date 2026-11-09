// Doors and area transitions in the region graph: the door and transition
// snapshots (with the retry until the door list is stable), map notes
// attached to their doors, the door on a nav edge or near a point, and the
// classification of an edge as clear, door or blocked.

#include "room_topology_internal.h"

#include "engine_reads.h"       // ExtractTextOrStrRef — transition-destination loc-string read
#include "log.h"
#include "spatial_change_detector.h"  // GetCachedWalls — seam-filtered perimeter cache
#include "transitions.h"        // IterateLandmarks + MarkLandmarkClaimedByDoor — landmark→door matching pass

namespace acc::room_topology {

namespace {

// Door-snapshot stability tracking. The initial SnapshotDoors call from
// BuildForArea races the engine's per-area object population: doors whose
// handles haven't been registered yet in CGameObjectArray are silently
// skipped by AreaObjectIterator (the resolver returns "miss" on an
// unregistered id, see project_csws_area_handles.md). Pre-fix we'd lock
// in that incomplete door set for the rest of the session and not even
// re-entering the area recovered — only a full game restart.
//
// MaybeRefreshDoors re-snapshots each tick until the count stays
// constant for `kDoorStabilityRequiredStreak` ticks in a row, then
// commits. A hard cap exists for the edge case of script-spawned doors
// that genuinely arrive late: rather than retry forever, we commit
// whatever we have and log that we hit the cap.
//
// Cluster-classification (ClassifyCluster's FindDoorOnEdge calls) runs
// once during BuildForArea against whatever door set was visible at
// that moment. Door-set updates after the initial build don't
// retro-classify clusters. This is accepted lossy behaviour: any
// late-arriving door still shows up as a "Tür" exit through the
// per-edge query, and the worst case is that a cluster's classification
// is one door short — strictly better than missing the door entirely as
// today.
constexpr int kDoorStabilityRequiredStreak = 2;
constexpr int kDoorRetryCapTicks           = 60;

// Find the door nearest to `p` within `maxDistM`. Returns the door
// index or -1. Used as the belt-and-braces fallback in ClassifyEdge:
// if SegmentCrossesWalkmesh reports a wall hit AND a door lives within
// 1m of the hit point, treat the edge as Door rather than Blocked —
// catches the rare "door inside solid walkmesh" authoring case.
int FindDoorNearPoint(const Vector& p, float maxDistM) {
    if (g_graph.door_count <= 0) return -1;
    float maxSq  = maxDistM * maxDistM;
    int   bestIx = -1;
    float bestSq = maxSq;
    for (int i = 0; i < g_graph.door_count; ++i) {
        float d2 = DistXYSq(g_graph.doors[i].pos, p);
        if (d2 <= bestSq) { bestSq = d2; bestIx = i; }
    }
    return bestIx;
}

}  // namespace

// Door snapshot + edge-near-door geometry.

// Collect every CSWSDoor in the area into g_graph.doors. Reads world
// position (+0x90) + transition destination (CExoLocString @+0x3c8 if
// non-empty). Idempotent within a BuildForArea call — relies on the
// caller to have Reset'd the cache.
//
// All reads are SEH-bounded through the underlying engine helpers.
// Faulted reads just skip the door; the snapshot truncates rather than
// fails so a partial set is still usable for the edge-near-door query.
//
// Collection-only — no logging. Callers handle the summary line (cadence
// differs between the initial build and the per-tick refresh loop) and
// the per-door diagnostic dump via LogDoorSnapshotDetails.
void SnapshotDoors(void* area) {
    // Landmark attribution survives a re-snapshot.
    //
    // This function owns the whole DoorRecord lifecycle, including the
    // one field it does not itself produce: landmarkName is written by
    // AttachLandmarksToDoors, which runs once per area during
    // BuildForArea. MaybeRefreshDoors re-snapshots on later ticks while
    // the door set settles, and re-running the attachment there would
    // re-log and re-claim every landmark every tick. So the names are
    // carried across by position instead — doors do not move.
    //
    // Keeping this here rather than in the caller is the point: whoever
    // rebuilds the array is responsible for not destroying data it does
    // not own. On a fresh BuildForArea the preceding Reset() has already
    // zeroed door_count, so nothing is carried across an area change —
    // which is exactly right, since the previous area's landmark names
    // must not leak into the new one.
    struct SavedLandmark { Vector pos; char name[64]; };
    SavedLandmark saved[kMaxDoors];
    int savedCount = g_graph.door_count;
    if (savedCount > kMaxDoors) savedCount = kMaxDoors;
    for (int i = 0; i < savedCount; ++i) {
        saved[i].pos = g_graph.doors[i].pos;
        std::strncpy(saved[i].name, g_graph.doors[i].landmarkName,
                     sizeof(saved[i].name) - 1);
        saved[i].name[sizeof(saved[i].name) - 1] = '\0';
    }

    g_graph.door_count = 0;
    if (!area) return;

    bool truncated = false;
    acc::engine::ForEachDoor(area, [&](void* obj, const Vector& pos) {
        if (g_graph.door_count >= kMaxDoors) {
            if (!truncated) {
                acclog::Write("WallTopo",
                              "SnapshotDoors: hit kMaxDoors=%d — truncating",
                              kMaxDoors);
            }
            truncated = true;
            return;
        }
        // Cosmetic doors are non-interactive set dressing — the engine
        // offers no actions on them. In the room-shape system a cosmetic
        // door at the end of a corridor otherwise reads as a "Tür" exit,
        // sending the player hunting for a way through that doesn't exist.
        // Skip them here so they never enter the door snapshot: the edge
        // then classifies on walkmesh geometry alone (dead end / wall),
        // which is the truth. Real doors are unaffected — including the K2
        // static-but-locked ones IsDoorCosmetic re-classifies as real; those
        // ARE a way through (once unlocked) and must read as doors.
        if (acc::engine::IsDoorCosmetic(obj)) return;

        DoorRecord& rec = g_graph.doors[g_graph.door_count];
        rec.pos                 = pos;
        rec.transitionDest[0]   = '\0';
        rec.locName[0]          = '\0';
        rec.landmarkName[0]     = '\0';
        // Transition destination CExoLocString lives at +0x3c8 (text)
        // with the strref at +0x3cc; ExtractTextOrStrRef walks both.
        // Empty result is fine — most doors aren't transitions.
        acc::engine::ExtractTextOrStrRef(
            obj,
            kDoorTransitionDestOffset,
            kDoorTransitionDestOffset + 4,
            rec.transitionDest, sizeof(rec.transitionDest));
        // CSWSDoor.loc_name @+0x39c — same CExoLocString layout. Used
        // as the door noun in cluster labels (Pillar 4 cycle narration
        // already reads this field). Empty for unnamed doors → caller
        // falls back to the localized generic word.
        acc::engine::ExtractTextOrStrRef(
            obj,
            kDoorLocNameOffset,
            kDoorLocNameOffset + 4,
            rec.locName, sizeof(rec.locName));
        ++g_graph.door_count;
    });

    // Restore the landmark attribution saved above. A door that already
    // carries a name this pass keeps it; otherwise match by position
    // (<0.1m) against the pre-snapshot set.
    for (int i = 0; i < g_graph.door_count; ++i) {
        if (g_graph.doors[i].landmarkName[0]) continue;  // freshly set, keep
        for (int j = 0; j < savedCount; ++j) {
            if (!saved[j].name[0]) continue;
            if (DistXYZSq(saved[j].pos, g_graph.doors[i].pos) < 0.01f) {  // same door (<0.1m)
                std::strncpy(g_graph.doors[i].landmarkName, saved[j].name,
                             sizeof(g_graph.doors[i].landmarkName) - 1);
                g_graph.doors[i]
                    .landmarkName[sizeof(g_graph.doors[i].landmarkName) - 1]
                    = '\0';
                break;
            }
        }
    }
}

// Collect every walk-through area-transition trigger into
// g_graph.transitions. Mirrors SnapshotDoors: kind filter + SEH-bounded
// reads, partial set on fault. Requires g_graph.node_pos to be populated
// (BuildForArea fills nodes before calling us) so each record can bind to
// its nearest nav node up front.
//
// Traps, shield triggers without a destination, and script-only triggers
// fail IsTransitionTrigger and are skipped — only triggers the engine
// would actually use to leave the area get voiced as exits.
void SnapshotTransitionTriggers(void* area) {
    g_graph.transition_count = 0;
    if (!area) return;

    acc::engine::AreaObjectIterator iter(area);
    void* obj = nullptr;
    while ((obj = iter.Next()) != nullptr) {
        if (g_graph.transition_count >= kMaxTransitions) {
            acclog::Write("WallTopo",
                          "SnapshotTransitions: hit kMaxTransitions=%d — "
                          "truncating", kMaxTransitions);
            break;
        }
        int kind = acc::engine::GetObjectKind(obj);
        if (kind != static_cast<int>(acc::engine::GameObjectKind::Trigger)) {
            continue;
        }
        if (!acc::engine::IsTransitionTrigger(obj)) continue;
        Vector origin;
        if (!acc::engine::GetObjectPosition(obj, origin)) continue;

        // The object position is only the trigger polygon's ORIGIN (the
        // GIT XPosition/YPosition the Geometry list is authored against),
        // not where the player crosses it. Transition polygons are long
        // strips across a corridor mouth, so the origin can sit many
        // metres from the crossing (Wohnmodul 082 Ost: origin
        // 8m east of the actual crossing, voiced as "Übergang Ost" for a
        // south-west exit). The polygon centroid is what the bearing and
        // the nearest-node attach must use; origin only as fallback.
        Vector pos = origin;
        Vector poly[32];
        int    nVerts = acc::engine::GetTriggerGeometry(obj, poly, 32);
        if (nVerts >= 3) {
            Vector sum = {0, 0, 0};
            for (int v = 0; v < nVerts; ++v) {
                sum.x += poly[v].x; sum.y += poly[v].y; sum.z += poly[v].z;
            }
            pos.x = sum.x / nVerts; pos.y = sum.y / nVerts;
            pos.z = sum.z / nVerts;
        }

        TransitionRecord& rec = g_graph.transitions[g_graph.transition_count];
        rec.pos         = pos;
        rec.destName[0] = '\0';
        rec.nearestNode = -1;
        acc::engine::ExtractTextOrStrRef(
            obj,
            kTriggerTransitionDestOffset,
            kTriggerTransitionDestOffset + 4,
            rec.destName, sizeof(rec.destName));
        if (!rec.destName[0]) continue;  // unreadable destination — skip

        float bestSq = kTransitionAttachMaxM * kTransitionAttachMaxM;
        for (int i = 0; i < g_graph.node_count; ++i) {
            float d2 = DistXYSq(g_graph.node_pos[i], pos);
            if (d2 < bestSq) { bestSq = d2; rec.nearestNode = i; }
        }

        acclog::Write("WallTopo",
                      "SnapshotTransitions: transition[%d] pos=(%.1f,%.1f) "
                      "(polygon centroid of %d verts; origin=(%.1f,%.1f)) "
                      "dest=\"%s\" nearestNode=%d (dist=%.1fm)",
                      g_graph.transition_count, pos.x, pos.y, nVerts,
                      origin.x, origin.y, rec.destName,
                      rec.nearestNode,
                      rec.nearestNode >= 0 ? std::sqrt(bestSq) : -1.0f);
        if (rec.nearestNode < 0) {
            acclog::Write("WallTopo",
                          "SnapshotTransitions: transition[%d] UNATTACHED — "
                          "no nav node within %.1fm",
                          g_graph.transition_count, kTransitionAttachMaxM);
        }
        ++g_graph.transition_count;
    }
    acclog::Write("WallTopo", "SnapshotTransitions: collected %d transition "
                  "trigger%s", g_graph.transition_count,
                  g_graph.transition_count == 1 ? "" : "s");
}

// Door-on-edge test. Per design choice (c): door is "on" the segment
// AB iff its projection parameter t is in [0,1] AND its perpendicular
// distance from the segment is ≤ kMaxPerpM. Pure 2D — the engine drops
// z for path solving and so do we.
//
// Tuned values:
//   - kMaxPerpM = 1.5m  — KOTOR door panels are ~2m wide; 1.5m gives
//     us slack for nav points that hug the door frame on either side
//     without admitting doors in adjacent rooms 4m away.
//   - kEndpointSlackM = 1.0m — admits doors that sit slightly past
//     either endpoint, common when level designers place the door's
//     centre a metre or so beyond the flanking nav point.
//
// Returns the door index on hit (>=0), or -1 if no door qualifies. On
// multi-door hits returns the first match — typical edges only ever
// cross one door, so a more sophisticated picker isn't worth the code.
int FindDoorOnEdge(const Vector& a, const Vector& b) {
    if (g_graph.door_count <= 0) return -1;

    constexpr float kMaxPerpM       = 1.5f;
    constexpr float kEndpointSlackM = 1.0f;

    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float abLenSq = abx * abx + aby * aby;
    if (abLenSq < 0.04f) {  // degenerate (<0.2m) — bail
        return -1;
    }
    float abLen      = std::sqrt(abLenSq);
    float slackT     = kEndpointSlackM / abLen;
    float maxPerpSq  = kMaxPerpM * kMaxPerpM;

    for (int i = 0; i < g_graph.door_count; ++i) {
        const Vector& d = g_graph.doors[i].pos;
        float adx = d.x - a.x;
        float ady = d.y - a.y;
        float t = (adx * abx + ady * aby) / abLenSq;
        if (t < -slackT || t > 1.0f + slackT) continue;
        float closestX = a.x + t * abx;
        float closestY = a.y + t * aby;
        float dxC = d.x - closestX;
        float dyC = d.y - closestY;
        float perpSq = dxC * dxC + dyC * dyC;
        if (perpSq <= maxPerpSq) {
            return i;
        }
    }
    return -1;
}

// Classify a nav-graph segment AB against walls + doors. Optional
// `areaForDiag` enables the caveat-2 cross-room logging — pass nullptr
// when caveat-2 noise would dominate (e.g. corridor-span tests where
// crossing rooms is by design). Logs caveat-1 hits unconditionally
// because they're rare and high-signal.
//
// Multi-fire note: a graph edge (i, j) may be classified from multiple
// passes (Pass 1 core-merge + Pass 2 absorb vetoes, then Pass 3
// external-edge collection). All
// fires hit the same logic and tally the same counters; the summary
// line documents the multiplication so a reader doesn't mistake it for
// per-edge truth. Caveat-1 / caveat-2 LOG ENTRIES include their caller
// context so the same edge appearing multiple times reads naturally.
EdgeResult ClassifyEdge(void* areaForDiag,
                        const Vector& a, const Vector& b,
                        const char* callerCtx) {
    EdgeResult r{kEdgeClear, -1};

    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    bool haveWalls = acc::spatial::change_detector::GetCachedWalls(
        walls, wallCount);
    Vector hit{};
    bool wallHit = haveWalls &&
                   acc::engine::SegmentCrossesWalkmesh(walls, wallCount,
                                                      a, b, hit);

    if (wallHit) {
        // Caveat 1: walkmesh hole convention says doors sit in
        // walkable gaps. If the wall hit is within 1m of a known
        // door, the convention failed for this door (or the seam
        // filter mis-classified a door-frame edge). Reclassify as
        // Door so we keep the actionable label.
        int nearestDoor = FindDoorNearPoint(hit, 1.0f);
        if (nearestDoor >= 0) {
            ++s_caveat1_hits;
            float ddx = g_graph.doors[nearestDoor].pos.x - hit.x;
            float ddy = g_graph.doors[nearestDoor].pos.y - hit.y;
            float dist = std::sqrt(ddx * ddx + ddy * ddy);
            acclog::Write(
                "WallTopo",
                "ClassifyEdge HOLE-IN-WALL [%s] — segment (%.1f,%.1f) -> "
                "(%.1f,%.1f): wall hit at (%.1f,%.1f), door[%d] at "
                "(%.1f,%.1f) %.2fm away, transition=\"%s\" → "
                "reclassifying Blocked → Door",
                callerCtx ? callerCtx : "?",
                a.x, a.y, b.x, b.y,
                hit.x, hit.y,
                nearestDoor,
                g_graph.doors[nearestDoor].pos.x,
                g_graph.doors[nearestDoor].pos.y,
                dist,
                g_graph.doors[nearestDoor].transitionDest[0]
                    ? g_graph.doors[nearestDoor].transitionDest
                    : "(none)");
            r.kind    = kEdgeDoor;
            r.doorIdx = nearestDoor;
            ++s_class_door;
            return r;
        }
        r.kind = kEdgeBlocked;
        ++s_class_blocked;
        return r;
    }

    // Walkmesh-clear. Check whether a door sits on the segment for
    // label rewriting.
    int doorIdx = FindDoorOnEdge(a, b);
    if (doorIdx >= 0) {
        r.kind    = kEdgeDoor;
        r.doorIdx = doorIdx;
        ++s_class_door;
        return r;
    }

    // Clear, no door. Caveat 2: log when the two endpoints sit in
    // different .wok rooms. Noisy in hubs (plazas span 4 .wok rooms),
    // but the pattern of WHICH edges fire repeatedly across areas is
    // the diagnostic signal — see commit comment for the read pattern.
    if (areaForDiag) {
        int roomA = -1, roomB = -1;
        acc::engine::GetRoomAtIndexed(areaForDiag, a, roomA);
        acc::engine::GetRoomAtIndexed(areaForDiag, b, roomB);
        if (roomA != roomB && roomA >= 0 && roomB >= 0) {
            ++s_caveat2_hits;
            float ex = b.x - a.x, ey = b.y - a.y;
            float dist = std::sqrt(ex * ex + ey * ey);
            acclog::Write(
                "WallTopo",
                "ClassifyEdge CROSS-ROOM-CLEAR [%s] — (%.1f,%.1f,room=%d) "
                "-> (%.1f,%.1f,room=%d), distance=%.1fm, no wall, no door. "
                "Possible undetected archway / .wok seam noise.",
                callerCtx ? callerCtx : "?",
                a.x, a.y, roomA, b.x, b.y, roomB, dist);
        }
    }

    ++s_class_clear;
    return r;
}

// Match each landmark waypoint (registered in transitions.cpp's per-area
// landmark cache during RebuildLandmarkCache) to the nearest door within
// kLandmarkDoorMatchMaxM. When a match lands, the door's landmarkName is
// populated and the landmark is flagged via MarkLandmarkClaimedByDoor so
// the proximity-fire path won't re-announce the same name a second later.
//
// Greedy first-come — if two landmarks contest the same door (rare in
// vanilla content, only seen at hub doors with multiple co-located
// waypoints), the first-iterated one wins and the second logs a
// conflict line. Tuning the rule beyond first-come needs evidence first.
//
// Threshold rationale: 5.0m. Full-module
// extraction of both games' GITs: median map-note→nearest-door distance is
// 5.2m in K1 and 9.1m in K2 — K2 authors place notes behind the doorway
// (room side), not on it, so 3.0m attached only 14% of K2 notes (36% K1).
// 5.0m lifts that to ~26% / ~49%. Wrong-door risk stays bounded: 90% of
// doors have no neighbour door within ~5–6m (door nearest-neighbour p10),
// so the nearest-door pick is still effectively unambiguous; the CONFLICT
// and UNMATCHED log lines below remain the watchdog for the rest.
// Original 3.0m anchor case still passes: "Zur Oberstadt" landmark
// @ (112.61, 83.34) ↔ door[4] @ (112.5, 81.8) → 1.5m.
//
// Diagnostics: per-landmark match / unmatched line + summary line for
// rate analysis. Unmatched lines include the nearest-door distance so
// post-mortem can tell whether the threshold needs widening for a
// particular area or whether the landmark genuinely isn't door-shaped.
void AttachLandmarksToDoors(void* area) {
    constexpr float kLandmarkDoorMatchMaxM = 5.0f;
    constexpr float kMaxSq = kLandmarkDoorMatchMaxM * kLandmarkDoorMatchMaxM;

    if (g_graph.door_count <= 0) {
        acclog::Write("WallTopo",
                      "AttachLandmarks: no doors snapshotted — skipping");
        return;
    }

    int matched = 0, unmatched = 0, conflicts = 0;

    int cursor = 0;
    char name[128] = {0};
    Vector lmPos = {0.0f, 0.0f, 0.0f};
    int landmarkIdx = -1;
    while (acc::transitions::IterateLandmarks(
               area, cursor, name, sizeof(name), lmPos, landmarkIdx)) {
        // Linear scan over doors — door_count is tiny (≤128, in practice
        // <30 for vanilla areas), so the O(landmarks * doors) sweep is
        // a few hundred multiplies total per area build.
        int   bestDoor = -1;
        float bestSq   = 1e30f;
        for (int d = 0; d < g_graph.door_count; ++d) {
            float dsq = DistXYSq(g_graph.doors[d].pos, lmPos);
            if (dsq < bestSq) {
                bestSq   = dsq;
                bestDoor = d;
            }
        }

        if (bestDoor < 0) continue;
        float bestDist = std::sqrt(bestSq);

        if (bestSq > kMaxSq) {
            // Nearest door beyond threshold — keep landmark out-of-band
            // for the proximity-fire path. Log the candidate distance so
            // post-mortem can tune the threshold per-area if needed.
            ++unmatched;
            acclog::Write(
                "WallTopo",
                "AttachLandmarks: UNMATCHED landmark[%d] '%s' "
                "lmPos=(%.2f,%.2f) — nearest door[%d] pos=(%.2f,%.2f) "
                "dist=%.2fm > %.1fm threshold (transition=\"%s\")",
                landmarkIdx, name,
                lmPos.x, lmPos.y,
                bestDoor,
                g_graph.doors[bestDoor].pos.x, g_graph.doors[bestDoor].pos.y,
                bestDist, kLandmarkDoorMatchMaxM,
                g_graph.doors[bestDoor].transitionDest[0]
                    ? g_graph.doors[bestDoor].transitionDest
                    : "(none)");
            continue;
        }

        // Within threshold. Greedy claim: first-iterated landmark wins.
        if (g_graph.doors[bestDoor].landmarkName[0] != '\0') {
            ++conflicts;
            acclog::Write(
                "WallTopo",
                "AttachLandmarks: CONFLICT landmark[%d] '%s' "
                "would match door[%d] (dist=%.2fm) but door already "
                "claimed by '%s' — skipping",
                landmarkIdx, name, bestDoor, bestDist,
                g_graph.doors[bestDoor].landmarkName);
            continue;
        }

        std::strncpy(g_graph.doors[bestDoor].landmarkName, name,
                     sizeof(g_graph.doors[bestDoor].landmarkName) - 1);
        g_graph.doors[bestDoor]
            .landmarkName[sizeof(g_graph.doors[bestDoor].landmarkName) - 1] = '\0';
        acc::transitions::MarkLandmarkClaimedByDoor(landmarkIdx);
        ++matched;
        acclog::Write(
            "WallTopo",
            "AttachLandmarks: matched landmark[%d] '%s' → door[%d] "
            "lmPos=(%.2f,%.2f) doorPos=(%.2f,%.2f) dist=%.2fm "
            "(was transitionDest=\"%s\")",
            landmarkIdx, name, bestDoor,
            lmPos.x, lmPos.y,
            g_graph.doors[bestDoor].pos.x, g_graph.doors[bestDoor].pos.y,
            bestDist,
            g_graph.doors[bestDoor].transitionDest[0]
                ? g_graph.doors[bestDoor].transitionDest
                : "(none)");
    }

    acclog::Write(
        "WallTopo",
        "AttachLandmarks: summary — matched=%d unmatched=%d conflicts=%d "
        "(landmarks scanned=%d, doors=%d, threshold=%.1fm)",
        matched, unmatched, conflicts,
        matched + unmatched + conflicts, g_graph.door_count,
        kLandmarkDoorMatchMaxM);
}

void MaybeRefreshDoors(void* area) {
    if (!HasGraphForArea(area))      return;
    if (g_doors_stability.committed) return;

    int prevCount = g_graph.door_count;

    // SnapshotDoors preserves landmark attribution itself — it owns the
    // DoorRecord lifecycle, so this caller does not have to save and
    // restore the field around it any more.
    SnapshotDoors(area);

    ++g_doors_stability.retry_ticks;

    if (g_graph.door_count == g_doors_stability.last_count) {
        ++g_doors_stability.streak;
    } else {
        // Door set changed — log the new details so we can grep
        // exactly what arrived (or, less likely, what disappeared).
        acclog::Write("WallTopo",
                      "MaybeRefreshDoors: count changed %d -> %d at retry tick %d",
                      prevCount, g_graph.door_count,
                      g_doors_stability.retry_ticks);
        LogDoorSnapshotDetails(area);
        g_doors_stability.last_count = g_graph.door_count;
        g_doors_stability.streak     = 1;
    }

    bool stable  = g_doors_stability.streak >= kDoorStabilityRequiredStreak;
    bool capHit  = g_doors_stability.retry_ticks >= kDoorRetryCapTicks;
    if (stable || capHit) {
        g_doors_stability.committed = true;
        acclog::Write("WallTopo",
                      "MaybeRefreshDoors: %s — final door_count=%d after %d retry tick%s",
                      capHit && !stable ? "cap reached" : "stable",
                      g_graph.door_count,
                      g_doors_stability.retry_ticks,
                      g_doors_stability.retry_ticks == 1 ? "" : "s");
    }
}

}  // namespace acc::room_topology
