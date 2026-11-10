// Doors and area transitions in the region graph: the door and transition
// snapshots (with the retry until the door list is stable), map notes
// attached to their doors, the door on a nav edge or near a point, and the
// classification of an edge as clear, door or blocked.

#include "room_topology_internal.h"

#include "engine_reads.h"           // ExtractTextOrStrRef
#include "log.h"
#include "spatial_wall_surfaces.h"  // GetCachedWalls
#include "transitions.h"            // IterateLandmarks, MarkLandmarkClaimedByDoor

namespace acc::room_topology {

namespace {

// The door list of an area fills in over the first ticks: a door whose
// handle is not yet registered in CGameObjectArray is skipped by the object
// iterator. MaybeRefreshDoors therefore re-snapshots every tick until the
// count has been the same for this many ticks in a row, and stops waiting at
// the cap (script-spawned doors that arrive late).
//
// Clusters are classified once, in BuildForArea, against the doors known
// then. A door that arrives later still shows as a door exit through the
// per-edge query; the cluster's own classification can be one door short.
constexpr int kDoorStabilityRequiredStreak = 2;
constexpr int kDoorRetryCapTicks           = 60;

const char* DestOrNone(const DoorRecord& door) {
    return door.transitionDest[0] ? door.transitionDest : "(none)";
}

// A CExoLocString field: inline text at `offset`, its strref four bytes on.
// Empty when the field is.
template <size_t N>
void ReadLocString(void* obj, size_t offset, char (&out)[N]) {
    out[0] = '\0';
    acc::engine::ExtractTextOrStrRef(obj, offset, offset + 4, out, N);
}

// The door nearest to p in the horizontal plane, -1 without doors; outSq
// gets its squared distance.
int NearestDoor(const Vector& p, float& outSq) {
    int best = -1;
    outSq = 1e30f;
    for (int i = 0; i < g_graph.door_count; ++i) {
        const float d2 = DistXYSq(g_graph.doors[i].pos, p);
        if (d2 < outSq) { outSq = d2; best = i; }
    }
    return best;
}

// Where a transition is crossed: the centroid of the trigger polygon. The
// object position is only the polygon's origin (the GIT position its
// Geometry list is authored against), and a transition polygon is a long
// strip across a corridor mouth, so the origin can sit metres from the
// crossing (Citadel Station module 082 East: 8 m east of it, voiced as
// "transition East" for a south-west exit). The origin without a polygon.
Vector TriggerCrossing(void* trigger, const Vector& origin, int& outVerts) {
    Vector poly[32];
    outVerts = acc::engine::GetTriggerGeometry(trigger, poly, 32);
    if (outVerts < 3) return origin;
    Vector sum = {0, 0, 0};
    for (int v = 0; v < outVerts; ++v) {
        sum.x += poly[v].x; sum.y += poly[v].y; sum.z += poly[v].z;
    }
    return {sum.x / outVerts, sum.y / outVerts, sum.z / outVerts};
}

}  // namespace

// Fills g_graph.doors with the area's real doors. A door that cannot be read
// is skipped, so a partial set stays usable. No logging: the callers differ
// in how often they want it.
//
// landmarkName is the one field not produced here (AttachLandmarksToDoors
// writes it once per area), so it is carried across a re-snapshot by
// position; doors do not move. After Reset() there is nothing to carry, so
// no name leaks into the next area.
void SnapshotDoors(void* area) {
    struct SavedLandmark { Vector pos; char name[64]; };
    SavedLandmark saved[kMaxDoors];
    const int savedCount = g_graph.door_count;
    for (int i = 0; i < savedCount; ++i) {
        saved[i].pos = g_graph.doors[i].pos;
        strncpy_s(saved[i].name, g_graph.doors[i].landmarkName, _TRUNCATE);
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
        // A cosmetic door is set dressing with no actions. Left in, one at
        // the end of a corridor reads as a "Door" exit and sends the player
        // looking for a way through that is not there; left out, the edge
        // classifies on the walkmesh alone. The K2 static-but-locked doors
        // that IsDoorCosmetic counts as real stay: they are a way through
        // once unlocked.
        if (acc::engine::IsDoorCosmetic(obj)) return;

        DoorRecord& rec = g_graph.doors[g_graph.door_count++];
        rec.pos             = pos;
        rec.landmarkName[0] = '\0';
        ReadLocString(obj, kDoorTransitionDestOffset, rec.transitionDest);
        ReadLocString(obj, kDoorLocNameOffset, rec.locName);
    });

    for (int i = 0; i < g_graph.door_count; ++i) {
        for (int j = 0; j < savedCount; ++j) {
            if (!saved[j].name[0]) continue;
            if (DistXYZSq(saved[j].pos, g_graph.doors[i].pos) < 0.01f) {  // same door (<0.1m)
                strncpy_s(g_graph.doors[i].landmarkName, saved[j].name, _TRUNCATE);
                break;
            }
        }
    }
}

// Fills g_graph.transitions with the walk-through area transitions, each
// bound to its nearest nav node; BuildForArea has filled the nodes by now.
// Traps and triggers without a destination fail IsTransitionTrigger: only
// what the engine would use to leave the area is voiced as an exit.
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
        if (acc::engine::GetObjectKind(obj) !=
            static_cast<int>(acc::engine::GameObjectKind::Trigger)) {
            continue;
        }
        if (!acc::engine::IsTransitionTrigger(obj)) continue;
        Vector origin;
        if (!acc::engine::GetObjectPosition(obj, origin)) continue;

        int nVerts = 0;
        const Vector pos = TriggerCrossing(obj, origin, nVerts);

        TransitionRecord& rec = g_graph.transitions[g_graph.transition_count];
        rec.pos         = pos;
        rec.nearestNode = -1;
        ReadLocString(obj, kTriggerTransitionDestOffset, rec.destName);
        if (!rec.destName[0]) continue;  // unreadable destination

        float bestSq = kTransitionAttachMaxM * kTransitionAttachMaxM;
        for (int i = 0; i < g_graph.node_count; ++i) {
            const float d2 = DistXYSq(g_graph.node_pos[i], pos);
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

// The first door on segment AB, or -1: its projection falls on the segment
// and it lies within kMaxPerpM of it. Horizontal only, like the engine's
// path solving.
//
//   - kMaxPerpM 1.5 m: a door panel is about 2 m wide. This leaves room for
//     nav points that hug the frame without admitting a door in the next
//     room 4 m away.
//   - kEndpointSlackM 1.0 m: level designers often put a door's centre a
//     metre past the nav point that flanks it.
//
// An edge crosses one door at most in practice, so the first hit is taken.
int FindDoorOnEdge(const Vector& a, const Vector& b) {
    constexpr float kMaxPerpM       = 1.5f;
    constexpr float kEndpointSlackM = 1.0f;

    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float abLenSq = abx * abx + aby * aby;
    if (abLenSq < 0.04f) return -1;  // shorter than 0.2 m
    const float slackT    = kEndpointSlackM / std::sqrt(abLenSq);
    const float maxPerpSq = kMaxPerpM * kMaxPerpM;

    for (int i = 0; i < g_graph.door_count; ++i) {
        const Vector& d = g_graph.doors[i].pos;
        const float t = ((d.x - a.x) * abx + (d.y - a.y) * aby) / abLenSq;
        if (t < -slackT || t > 1.0f + slackT) continue;
        const float dxC = d.x - (a.x + t * abx);
        const float dyC = d.y - (a.y + t * aby);
        if (dxC * dxC + dyC * dyC <= maxPerpSq) return i;
    }
    return -1;
}

// Classifies nav segment AB against the walls and the doors. An edge is
// classified from several passes, and each call counts: the tallies in the
// summary line are per call, not per edge. callerCtx names the pass in the
// two diagnostic lines. areaForDiag enables the cross-room line; pass
// nullptr where crossing rooms is by design.
EdgeResult ClassifyEdge(void* areaForDiag,
                        const Vector& a, const Vector& b,
                        const char* callerCtx) {
    EdgeResult r{kEdgeClear, -1};

    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    Vector hit{};
    if (acc::spatial::wall_surfaces::GetCachedWalls(walls, wallCount) &&
        acc::engine::SegmentCrossesWalkmesh(walls, wallCount, a, b, hit)) {
        // Doors sit in walkable gaps of the walkmesh, as a rule. A wall hit
        // within 1 m of a door means the rule failed for that door (or the
        // seam filter took a door-frame edge for a wall): still a door.
        float doorSq = 0.0f;
        const int door = NearestDoor(hit, doorSq);
        if (door < 0 || doorSq > 1.0f) {
            r.kind = kEdgeBlocked;
            ++s_class_blocked;
            return r;
        }
        ++s_caveat1_hits;
        acclog::Write(
            "WallTopo",
            "ClassifyEdge HOLE-IN-WALL [%s] — segment (%.1f,%.1f) -> "
            "(%.1f,%.1f): wall hit at (%.1f,%.1f), door[%d] at "
            "(%.1f,%.1f) %.2fm away, transition=\"%s\" → "
            "reclassifying Blocked → Door",
            callerCtx, a.x, a.y, b.x, b.y, hit.x, hit.y, door,
            g_graph.doors[door].pos.x, g_graph.doors[door].pos.y,
            std::sqrt(doorSq), DestOrNone(g_graph.doors[door]));
        r.kind    = kEdgeDoor;
        r.doorIdx = door;
        ++s_class_door;
        return r;
    }

    const int doorIdx = FindDoorOnEdge(a, b);
    if (doorIdx >= 0) {
        r.kind    = kEdgeDoor;
        r.doorIdx = doorIdx;
        ++s_class_door;
        return r;
    }

    // Clear and no door, yet the endpoints lie in different .wok rooms:
    // logged. Noisy in hubs (a plaza spans four rooms); which edges recur
    // across areas is the signal.
    if (areaForDiag) {
        int roomA = -1, roomB = -1;
        acc::engine::GetRoomAtIndexed(areaForDiag, a, roomA);
        acc::engine::GetRoomAtIndexed(areaForDiag, b, roomB);
        if (roomA != roomB && roomA >= 0 && roomB >= 0) {
            ++s_caveat2_hits;
            acclog::Write(
                "WallTopo",
                "ClassifyEdge CROSS-ROOM-CLEAR [%s] — (%.1f,%.1f,room=%d) "
                "-> (%.1f,%.1f,room=%d), distance=%.1fm, no wall, no door. "
                "Possible undetected archway / .wok seam noise.",
                callerCtx, a.x, a.y, roomA, b.x, b.y, roomB, DistXY(a, b));
        }
    }

    ++s_class_clear;
    return r;
}

// Gives each map note to the nearest door within kLandmarkDoorMatchMaxM: the
// door gets the note's name, and the note is marked claimed so the proximity
// announce does not say the same name a second later. The first note to
// reach a door keeps it; a second one logs a conflict.
//
// Why 5 m: over every module of both games the median distance from a map
// note to its nearest door is 5.2 m in K1 and 9.1 m in K2, where authors put
// the note behind the doorway rather than on it. 3 m attached 36% of the K1
// notes and 14% of K2's; 5 m about 49% and 26%. The wrong door stays
// unlikely: 90% of doors have no other door within 5 to 6 m. The CONFLICT
// and UNMATCHED lines are the watchdog for the rest; an unmatched line
// carries the distance, to tell a tight threshold from a note that is not
// at a door at all.
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
        float bestSq = 0.0f;
        const int bestDoor = NearestDoor(lmPos, bestSq);
        if (bestDoor < 0) continue;
        DoorRecord& door = g_graph.doors[bestDoor];
        const float bestDist = std::sqrt(bestSq);

        if (bestSq > kMaxSq) {
            ++unmatched;
            acclog::Write(
                "WallTopo",
                "AttachLandmarks: UNMATCHED landmark[%d] '%s' "
                "lmPos=(%.2f,%.2f) — nearest door[%d] pos=(%.2f,%.2f) "
                "dist=%.2fm > %.1fm threshold (transition=\"%s\")",
                landmarkIdx, name, lmPos.x, lmPos.y, bestDoor,
                door.pos.x, door.pos.y, bestDist, kLandmarkDoorMatchMaxM,
                DestOrNone(door));
            continue;
        }

        if (door.landmarkName[0] != '\0') {
            ++conflicts;
            acclog::Write(
                "WallTopo",
                "AttachLandmarks: CONFLICT landmark[%d] '%s' "
                "would match door[%d] (dist=%.2fm) but door already "
                "claimed by '%s' — skipping",
                landmarkIdx, name, bestDoor, bestDist, door.landmarkName);
            continue;
        }

        strncpy_s(door.landmarkName, name, _TRUNCATE);
        acc::transitions::MarkLandmarkClaimedByDoor(landmarkIdx);
        ++matched;
        acclog::Write(
            "WallTopo",
            "AttachLandmarks: matched landmark[%d] '%s' → door[%d] "
            "lmPos=(%.2f,%.2f) doorPos=(%.2f,%.2f) dist=%.2fm "
            "(was transitionDest=\"%s\")",
            landmarkIdx, name, bestDoor, lmPos.x, lmPos.y,
            door.pos.x, door.pos.y, bestDist, DestOrNone(door));
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

    const int prevCount = g_graph.door_count;
    SnapshotDoors(area);
    ++g_doors_stability.retry_ticks;

    if (g_graph.door_count == g_doors_stability.last_count) {
        ++g_doors_stability.streak;
    } else {
        acclog::Write("WallTopo",
                      "MaybeRefreshDoors: count changed %d -> %d at retry tick %d",
                      prevCount, g_graph.door_count,
                      g_doors_stability.retry_ticks);
        LogDoorSnapshotDetails(area);
        g_doors_stability.last_count = g_graph.door_count;
        g_doors_stability.streak     = 1;
    }

    const bool stable = g_doors_stability.streak >= kDoorStabilityRequiredStreak;
    const bool capHit = g_doors_stability.retry_ticks >= kDoorRetryCapTicks;
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
