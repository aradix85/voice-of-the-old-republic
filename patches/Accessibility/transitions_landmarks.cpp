// Landmarks: the per-area cache of named map notes and placeables, the
// proximity announcement as the player walks past one, and the lookups
// other modules use (FindLandmarkNear, IterateLandmarks).

#include "transitions_internal.h"

#include "discovery.h"       // discovery-tier area reconciliation + landmark record
#include "engine_area.h"
#include "log.h"
#include "speech.h"
#include "room_topology.h"      // single source of truth for perceptual-
                                // region labels (nav-graph decomposition)

namespace acc::transitions {

namespace {

// Flat landmark cache. Built once on each area change by scanning
// every CSWSWaypoint with has_map_note != 0 AND map_note_enabled != 0
// and recording its world position + map_note CExoLocString text. Not
// keyed by .lyt-room: on K1's sliver-shaped rooms a player, cursor or
// marker inside one sliver would miss a landmark stored under the
// adjacent one. Lookups are proximity-based, via `FindLandmarkNear`.
//
// Fog-of-war respect: filtering on map_note_enabled prevents spoiling
// locations the player hasn't yet discovered on the in-game map. When
// the player walks into an unrevealed area, the relevant landmarks
// aren't in the cache; we fall back to room-name / shape tiers —
// same information channel the sighted player has.
//
// `doorMatched` is set by room_topology::AttachLandmarksToDoors when
// this landmark's name has been embedded in a cluster label.
// TickProximityLandmarks skips matched entries so the player doesn't
// hear the name inside the region announce and then again standalone a
// second later.
//
// Sized at kMaxLandmarks=128 — vanilla KOTOR areas have <50 landmark
// waypoints each. Cache is invalidated (zeroed) on every area change.
constexpr int kMaxLandmarks = 128;
struct Landmark {
    char     name[128];
    Vector   pos;
    bool     doorMatched;
    uint32_t handle;       // server handle of the source waypoint — for discovery::Record
};
Landmark g_landmarks[kMaxLandmarks];
int      g_landmark_count = 0;

// Which area the cache above currently describes. Set by
// RebuildLandmarkCache (including when it is handed a null area, so the
// "never built" and "built for nothing" states are distinguishable).
//
// This exists so the landmark→door matching pass can PROVE the cache is
// the one it wants instead of assuming it. AttachLandmarksToDoors is only
// correct if RebuildLandmarkCache already ran for the same area; without
// the check a wrong ordering would read as "0 landmarks matched",
// indistinguishable from an area without landmark waypoints.
void* g_landmark_area = nullptr;

// Landmark-cache staleness tracking. Map notes can be script-enabled long
// after area entry; a cache from the entry scan would keep such a note
// out of the proximity announce and discovery for the whole visit.
// g_landmark_enabled_at_scan records the enabled count seen by the last
// RebuildLandmarkCache; TickLandmarkCacheRecheck re-counts periodically
// and rebuilds on drift.
int g_landmark_enabled_at_scan = 0;

// Landmark proximity, decoupled from .lyt-room crossings: a landmark in a
// thin sliver room around a door would otherwise never fire. Each tick the
// nearest landmark within the enter range speaks once it has been stable
// for kLandmarkStabilityTicks and is not the last spoken one; leaving
// beyond the exit range re-arms it.
//
// The ranges scale per area with the fog-of-war cell size, so the mod
// reveals a landmark about when the sighted map does (UpdateMapData
// explores the party member's cell + 4 neighbours each tick; see
// ingame-screens-reference.md, fog_of_war_exploration_model). A flat 8 m
// fits interiors (Jedi Enclave cell 7.8 m) but falls far short on open
// maps (Dune Sea cell 44-48 m). The floor keeps 8 m / 12 m as the minimum;
// the exit range keeps the 1.5x hysteresis.
constexpr float kLandmarkEnterRangeFloorM = 8.0f;
constexpr float kLandmarkFogCellScale     = 1.5f;
constexpr float kLandmarkExitRatio        = 1.5f;
float g_lm_enter_range_m = kLandmarkEnterRangeFloorM;
float g_lm_exit_range_m  = kLandmarkEnterRangeFloorM * kLandmarkExitRatio;
constexpr int kLandmarkStabilityTicks = 5;
int g_lm_prox_pending_idx     = -1;
int g_lm_prox_pending_count   = 0;
int g_lm_prox_last_spoken_idx = -1;

// False while the last recompute ran without a live area map (early
// area-change tick); TickLandmarkCacheRecheck retries on its 1s cadence
// until the map resolves. Areas genuinely without a map stay on the
// floor via this same path — the retry is 4 field reads, negligible.
bool g_lm_ranges_from_fog = false;

// Cadence for TickLandmarkCacheRecheck (see g_landmark_enabled_at_scan
// above RebuildLandmarkCache for the staleness story).
constexpr DWORD kLandmarkRecheckMs = 1000;
DWORD g_landmark_recheck_last_ms  = 0;

// Proximity auto-discovery for script-gated map notes. Vanilla enables
// these via thin OnEnter tripwire triggers that assume centre-of-
// corridor walking: the temple entrance note in unk_m41ad is enabled by
// a ~1.5 m strip with a ~3 m walkable gap along the north wall, which a
// wall-following walk threads every pass. The Unknown World gates about
// half its notes this way (the other planets enable theirs from module
// entry). When the leader is standing at the spot, enable the note
// ourselves: the same engine write the tripwire script performs,
// one-way. The radius is tighter than the vanilla tripwires and below
// the landmark enter-range floor, so the proximity announce fires on
// the same recheck tick that surfaces the note. Flat on purpose: it
// stands in for stepping on a tripwire, not for seeing the map, so it
// does not scale with the fog cell size.
constexpr float kMapNoteAutoDiscoverRangeM = 5.0f;

void RecomputeLandmarkRanges() {
    float cellX = 0.0f, cellY = 0.0f;
    float enter = kLandmarkEnterRangeFloorM;
    bool  haveCell = acc::engine::GetFogCellSizeM(
        acc::engine::GetAreaMap(), cellX, cellY);
    g_lm_ranges_from_fog = haveCell;
    if (haveCell) {
        float cell   = cellX > cellY ? cellX : cellY;
        float scaled = kLandmarkFogCellScale * cell;
        if (scaled > enter) enter = scaled;
    }
    g_lm_enter_range_m = enter;
    g_lm_exit_range_m  = enter * kLandmarkExitRatio;
    acclog::Write("Transition",
                  "landmark ranges: enter=%.1fm exit=%.1fm "
                  "(fog cell %.1fx%.1fm, %s)",
                  g_lm_enter_range_m, g_lm_exit_range_m, cellX, cellY,
                  haveCell ? "per-area" : "no area map -> floor");
}

// If `waypoint` (a landmark whose note is still disabled) sits within
// auto-discover range of the leader, enable it via the engine's own
// field write. Returns true when the note was enabled this call.
bool TryAutoDiscoverMapNote(void* waypoint, const Vector& playerPos) {
    Vector lmPos;
    if (!acc::engine::GetObjectPosition(waypoint, lmPos)) return false;
    float d2 = DistXYSq(playerPos, lmPos);
    if (d2 > kMapNoteAutoDiscoverRangeM * kMapNoteAutoDiscoverRangeM) {
        return false;
    }
    if (!acc::engine::EnableMapNote(waypoint)) return false;
    char note[128] = {0};
    acc::engine::GetWaypointMapNote(waypoint, note, sizeof(note));
    acclog::Write("Transition",
                  "landmark auto-discover: map note '%s' enabled at "
                  "%.2fm — leader reached the spot without crossing the "
                  "authored tripwire trigger",
                  note, std::sqrt(d2));
    return true;
}

}  // namespace

void RebuildLandmarkCache(void* area) {
    // Every entry below the count is written whole on insertion.
    g_landmark_count = 0;
    g_landmark_area  = area;

    if (!area) return;

    int scanned = 0, landmarks = 0, placed = 0, dropped = 0;
    acc::engine::AreaObjectIterator iter(area);
    void* obj = nullptr;
    while ((obj = iter.Next()) != nullptr) {
        ++scanned;
        int kind = acc::engine::GetObjectKind(obj);
        if (kind != static_cast<int>(
                acc::engine::GameObjectKind::Waypoint)) {
            continue;
        }
        // Two gates: must be a landmark (has_map_note bit) AND map note
        // currently enabled (engine fog-of-war model). The latter is the
        // spoiler-protection path; without it we'd surface labels for
        // unrevealed locations.
        if (!acc::engine::IsLandmarkWaypoint(obj))   continue;
        if (!acc::engine::IsMapNoteEnabled(obj))     continue;
        ++landmarks;

        Vector pos;
        if (!acc::engine::GetObjectPosition(obj, pos)) continue;

        char note[128] = {0};
        if (!acc::engine::GetWaypointMapNote(obj, note, sizeof(note))) {
            continue;
        }

        if (g_landmark_count >= kMaxLandmarks) {
            ++dropped;
            continue;
        }

        Landmark& lm = g_landmarks[g_landmark_count];
        std::strncpy(lm.name, note, sizeof(lm.name) - 1);
        lm.name[sizeof(lm.name) - 1] = '\0';
        lm.pos         = pos;
        lm.doorMatched = false;
        lm.handle      = acc::engine::GetObjectHandle(obj);
        acclog::Write("Transition",
                      "landmark[%d] '%s' pos=(%.2f,%.2f,%.2f)",
                      g_landmark_count, note, pos.x, pos.y, pos.z);
        ++g_landmark_count;
        ++placed;
    }

    if (dropped > 0) {
        acclog::Write("Transition",
                      "landmark cache overflow — %d landmarks dropped "
                      "(raise kMaxLandmarks above %d)",
                      dropped, kMaxLandmarks);
    }
    acclog::Write("Transition", "landmark cache rebuilt — scanned=%d landmarks=%d "
        "placed=%d (areaPtr=%p)",
        scanned, landmarks, placed, area);
    g_landmark_enabled_at_scan = landmarks;
    RecomputeLandmarkRanges();
}

// Detect map notes whose enabled flag flipped since the last cache scan
// and rebuild. Cheap: one flag sweep over the area object list, at most
// once per kLandmarkRecheckMs. On drift the proximity state is reset —
// cache indices are positional and a mid-list insertion would leave the
// pending/last-spoken indices pointing at the wrong entry.
// The same sweep performs proximity auto-discovery (see
// kMapNoteAutoDiscoverRangeM): a disabled note the leader is standing
// on is enabled in place and counts toward the drift check, so the
// cache rebuild + door attach + proximity announce all follow on this
// same tick.
void TickLandmarkCacheRecheck(void* area, const Vector& playerPos) {
    DWORD now = GetTickCount();
    if (g_landmark_recheck_last_ms != 0 &&
        (now - g_landmark_recheck_last_ms) < kLandmarkRecheckMs) {
        return;
    }
    g_landmark_recheck_last_ms = now;

    if (!g_lm_ranges_from_fog) RecomputeLandmarkRanges();

    int enabled = 0;
    acc::engine::AreaObjectIterator iter(area);
    while (void* obj = iter.Next()) {
        if (acc::engine::GetObjectKind(obj) != static_cast<int>(
                acc::engine::GameObjectKind::Waypoint)) {
            continue;
        }
        if (!acc::engine::IsLandmarkWaypoint(obj)) continue;
        if (!acc::engine::IsMapNoteEnabled(obj) &&
            !TryAutoDiscoverMapNote(obj, playerPos)) {
            continue;
        }
        ++enabled;
    }
    if (enabled == g_landmark_enabled_at_scan) return;

    acclog::Write("Transition",
                  "landmark recheck: enabled notes %d -> %d — rebuilding "
                  "cache (areaPtr=%p)",
                  g_landmark_enabled_at_scan, enabled, area);
    RebuildLandmarkCache(area);
    acc::room_topology::AttachLandmarksToDoors(area);
    g_lm_prox_pending_idx     = -1;
    g_lm_prox_pending_count   = 0;
    g_lm_prox_last_spoken_idx = -1;
}

// Per-tick scan over the landmark cache (the proximity note above). State:
// pending idx + pending count for stability, last-spoken idx for "already
// announced this approach".
void TickProximityLandmarks(const Vector& playerPos) {
    // Re-arm: if the last-spoken landmark is now beyond exit range,
    // clear it so we can re-announce on a future approach.
    if (g_lm_prox_last_spoken_idx >= 0) {
        int i = g_lm_prox_last_spoken_idx;
        if (i < g_landmark_count) {
            float d2 = DistXYSq(playerPos, g_landmarks[i].pos);
            if (d2 > g_lm_exit_range_m * g_lm_exit_range_m) {
                acclog::Write("Transition",
                              "landmark proximity re-arm idx=%d "
                              "(dist=%.2fm > %.1fm exit)",
                              i, std::sqrt(d2), g_lm_exit_range_m);
                g_lm_prox_last_spoken_idx = -1;
            }
        } else {
            // Stale index after area change.
            g_lm_prox_last_spoken_idx = -1;
        }
    }

    // Find the nearest landmark inside enter range. Skip entries whose
    // landmark room_topology already embedded in a cluster label — the
    // player heard the name as part of the room-shape announce on
    // cluster entry, and a redundant standalone fire would talk over
    // the next legitimate cue.
    int   nearest = -1;
    float bestD2  = g_lm_enter_range_m * g_lm_enter_range_m;
    for (int i = 0; i < g_landmark_count; ++i) {
        if (g_landmarks[i].name[0] == '\0') continue;
        if (g_landmarks[i].doorMatched)     continue;
        float d2 = DistXYSq(playerPos, g_landmarks[i].pos);
        if (d2 < bestD2) {
            bestD2  = d2;
            nearest = i;
        }
    }

    if (nearest < 0) {
        // No landmark in range — clear pending.
        g_lm_prox_pending_idx   = -1;
        g_lm_prox_pending_count = 0;
        return;
    }

    if (nearest == g_lm_prox_last_spoken_idx) {
        // Same landmark we already announced this approach.
        g_lm_prox_pending_idx   = -1;
        g_lm_prox_pending_count = 0;
        return;
    }

    if (nearest == g_lm_prox_pending_idx) {
        ++g_lm_prox_pending_count;
    } else {
        g_lm_prox_pending_idx   = nearest;
        g_lm_prox_pending_count = 1;
    }

    if (g_lm_prox_pending_count < kLandmarkStabilityTicks) return;

    // Fire — unless gated by combat / blocking UI. We still advance
    // last-spoken so the gate releasing doesn't trigger a burst of
    // stale landmarks for a player who stood still during combat.
    if (IsWorldSpeechGated()) {
        acclog::Write("Transition",
                      "landmark proximity -> '%s' (idx=%d dist=%.2fm) "
                      "gated, state advanced silently",
                      g_landmarks[nearest].name, nearest, std::sqrt(bestD2));
    } else {
        acc::speech::Speak(g_landmarks[nearest].name, /*interrupt=*/true);
        acclog::Write("Transition",
                      "landmark proximity -> '%s' (idx=%d dist=%.2fm)",
                      g_landmarks[nearest].name, nearest, std::sqrt(bestD2));
        // Organic discovery: the player walked close enough to hear this
        // landmark named. Record the source waypoint so the discovery-tier
        // cycle can resurface it. Only on the spoken (non-gated) path —
        // a silently-advanced gate means the player never heard it.
        void* lmObj = acc::engine::ResolveServerObjectHandle(
            g_landmarks[nearest].handle);
        if (lmObj) acc::discovery::Record(lmObj);
    }
    g_lm_prox_last_spoken_idx = nearest;
    g_lm_prox_pending_idx     = -1;
    g_lm_prox_pending_count   = 0;
}

bool FindLandmarkNear(const Vector& pos, float rangeM,
                      char* nameOut, size_t nameBufSize,
                      Vector& posOut,
                      int* outLandmarkIdx) {
    if (outLandmarkIdx) *outLandmarkIdx = -1;
    if (!nameOut || nameBufSize == 0) return false;
    nameOut[0] = '\0';
    if (rangeM <= 0.0f) return false;
    const float maxD2 = rangeM * rangeM;

    int   best   = -1;
    float bestD2 = maxD2;
    for (int i = 0; i < g_landmark_count; ++i) {
        if (g_landmarks[i].name[0] == '\0') continue;
        float d2 = DistXYSq(pos, g_landmarks[i].pos);
        if (d2 <= bestD2) {
            bestD2 = d2;
            best   = i;
        }
    }
    if (best < 0) return false;
    std::strncpy(nameOut, g_landmarks[best].name, nameBufSize - 1);
    nameOut[nameBufSize - 1] = '\0';
    posOut = g_landmarks[best].pos;
    if (outLandmarkIdx) *outLandmarkIdx = best;
    return true;
}

bool IterateLandmarks(void* area, int& cursor,
                      char* nameOut, size_t nameBufSize,
                      Vector& posOut, int& outLandmarkIdx) {
    if (!nameOut || nameBufSize == 0) return false;
    // The cache must belong to this area. On a mismatch yield nothing,
    // logged so it does not look like an area without landmarks; the
    // caller's walk loop exits on the first false, so this logs once per
    // walk.
    if (area != g_landmark_area) {
        acclog::Write("Transition",
                      "IterateLandmarks: cache belongs to area=%p but caller "
                      "asked for area=%p — RebuildLandmarkCache has not run "
                      "for this area; yielding no landmarks",
                      g_landmark_area, area);
        return false;
    }
    if (cursor < 0) cursor = 0;
    while (cursor < g_landmark_count) {
        int i = cursor++;
        if (g_landmarks[i].name[0] == '\0') continue;
        std::strncpy(nameOut, g_landmarks[i].name, nameBufSize - 1);
        nameOut[nameBufSize - 1] = '\0';
        posOut         = g_landmarks[i].pos;
        outLandmarkIdx = i;
        return true;
    }
    return false;
}

void MarkLandmarkClaimedByDoor(int landmarkIdx) {
    if (landmarkIdx < 0 || landmarkIdx >= g_landmark_count) return;
    g_landmarks[landmarkIdx].doorMatched = true;
}

// Landmark proximity firing + the recheck cadence timestamp.
void ResetLandmarkProximityState() {
    g_lm_prox_pending_idx      = -1;
    g_lm_prox_pending_count    = 0;
    g_lm_prox_last_spoken_idx  = -1;
    g_landmark_recheck_last_ms = 0;
}

}  // namespace acc::transitions
