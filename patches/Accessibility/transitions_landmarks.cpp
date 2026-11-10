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
// and recording its world position + map_note CExoLocString text. The
// cache is intentionally NOT keyed by .lyt-room — that's what this
// module used to do (`g_room_landmark[roomIdx]`) and the keying broke
// on K1's sliver-shaped rooms: a player / cursor / marker inside one
// sliver routinely failed to surface a landmark stored under the
// adjacent sliver. Lookups are proximity-based instead, via
// `FindLandmarkNear`.
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
// hear "Kreuzung, Ost, Tür Süd, Zur Oberstadt" followed by a redundant
// standalone "Zur Oberstadt" a second later.
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
// correct if RebuildLandmarkCache already ran for the same area; both call
// sites get that right by construction, and nothing enforced it. A wrong
// ordering used to present as a silent "0 landmarks matched" — a result
// indistinguishable from an area that genuinely has no landmark waypoints.
void* g_landmark_area = nullptr;

// Landmark-cache staleness tracking. Map notes can be script-enabled long
// after area entry (Shyrack-Höhle 2026-07-15: the 'Abtrünniger Sith'
// note was disabled at the 14:31 entry scan and enabled ~35 min later;
// the stale cache made it invisible to proximity announce + discovery
// for the whole visit). g_landmark_enabled_at_scan records the enabled
// count seen by the last RebuildLandmarkCache; TickLandmarkCacheRecheck
// re-counts periodically and rebuilds on drift.
int g_landmark_enabled_at_scan = 0;

// Defined below the landmark-proximity state block (which owns the
// range globals); rebuild triggers a per-area range recompute.
void RecomputeLandmarkRanges();

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
// corridor walking: the Tempeleingang note (unk_m41ad, 2026-07-16) is
// enabled by a ~1.5m-wide strip with a ~3m walkable gap along the north
// wall — the wall-following navigation style threads that gap every
// pass, and the player stood 10m from the note without ever discovering
// it. The Unknown World gates roughly half its notes this way (all
// other planets author notes enabled from module entry). When the
// leader is physically standing at the spot, enable the note ourselves
// — same engine write the tripwire script performs, one-way. Radius is
// deliberately tighter than the vanilla tripwires and below the
// landmark enter-range floor, so the proximity announce fires on the
// same recheck tick that surfaces the note. Kept flat on purpose (user
// decision 2026-07-17): it substitutes for stepping on a tripwire, not
// for seeing the map, so it does not scale with the fog cell size.
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
    // Reset cache. Use the index loop instead of memset so the Vector
    // member alignment guarantees survive any future struct refactor;
    // cheap (128 small entries).
    for (int i = 0; i < kMaxLandmarks; ++i) {
        g_landmarks[i].name[0]     = '\0';
        g_landmarks[i].pos         = {0, 0, 0};
        g_landmarks[i].doorMatched = false;
        g_landmarks[i].handle      = 0;
    }
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

// Per-tick scan over the landmark cache. Decoupled from .lyt-room
// crossings so landmarks whose room is a thin sliver still announce
// when the player walks close to the waypoint. Three-state machine
// mirroring turn_announce: pending idx + pending count for stability,
// last-spoken idx for "already announced this approach" suppression;
// last-spoken resets when the player walks past the per-area exit
// range so a return visit re-announces.
void TickProximityLandmarks(const Vector& playerPos) {
    // Re-arm: if the last-spoken landmark is now beyond exit range,
    // clear it so we can re-announce on a future approach.
    if (g_lm_prox_last_spoken_idx >= 0) {
        int i = g_lm_prox_last_spoken_idx;
        if (i >= 0 && i < g_landmark_count) {
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
    // the next legitimate cue (see project memory on the
    // cluster-vs-landmark double-announce regression).
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
    if (IsWorldSpeechGatedImpl()) {
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
    // Ordering contract, now checked rather than assumed. On a mismatch we
    // yield nothing — the same outcome as before — but it is logged instead
    // of looking like an area with no landmarks. Returning false on the
    // first call also means the caller's walk loop exits immediately, so
    // this logs once per attempt, not once per landmark.
    if (area != g_landmark_area) {
        acclog::Write("Transitions",
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
