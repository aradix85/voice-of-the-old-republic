// Internal to the transitions module (transitions*.cpp); no other file includes
// it. The public interface is transitions.h. Here: what the files share.

#pragma once

#include "transitions.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>


// Forward decl from core_dllmain.cpp. The OnSetMoveToModuleString detour
// below calls this so speech is loaded if a transition fires before any
// menu / focus event has run the same call.
void EnsureSpeechInitialized();

namespace acc::transitions {

// Module state. Both initialise to "no observation yet" sentinels — a
// nullptr area and a "none" cluster id. The first tick that resolves an
// area fires the area-change branch and sets prev_area_ptr, the first
// tick that resolves a labelled cluster fires the cluster-change branch.
// There is no separate first-tick suppression: the initial announce is
// the user's "you're now in {area} / {region}" orientation cue on game-
// load — silence here would be unhelpful per
// feedback_never_silence_fallback_announcement.
//
// Trigger moved off .lyt-room ids 2026-05-22 (see
// docs/room-shape-improvements.md). Empirical: room ids flipped on
// 94–98 % of player-step samples in dense Taris areas, forcing a text-
// dedup escape hatch to collapse the noise. `room_topology` cluster
// ids are spatially stable and correspond to perceptual regions —
// walking inside a corridor keeps the id constant, crossing into a
// junction flips it once. Cluster-change is the right signal.
extern void* g_prev_area;
extern int   g_prev_cluster_id;

// Latch covering the cutscene-load transient (see IsModuleLoadPending
// in the header for the why). Set in AnnouncePreLoadDestination from
// the OnSetMoveToModuleString entry hook; cleared in Tick() the next
// time we observe a fresh area pointer, or on player-loss reset.
extern bool  g_module_load_pending;

// Last friendly room name observed at the player position. Tracked so
// we ALSO fire on friendly-name change inside a single cluster — covers
// the TSL case where two adjacent .lyt-rooms share a cluster but have
// distinct human-readable names. K1 never trips this path in vanilla
// content (every room name is resref-style and filtered out by
// IsResrefStyleRoomName); the cost is a strcmp per tick.
extern char  g_prev_friendly_room_name[64];

// Stability dedup for the cluster trigger. Cluster boundaries flicker
// far less than .lyt-room boundaries (the player needs ~2-3m of motion
// to cross a real cluster boundary vs ~0.3m for some rooms), but a
// short stability window still smooths the rare snap-flip at a
// boundary. ~80ms at 60fps — too short to feel laggy, long enough to
// absorb a single-tick boundary glitch.
constexpr int kClusterStabilityTicks = 5;
extern int   g_pending_cluster_id;
extern int   g_pending_cluster_count;

// Centre rule (Taris Upper City). A Kreuzung, merged room or Bereich under
// Großer-Bereich size speaks exit words measured from its centroid, but the
// cluster is entered when its node becomes the nearest one, roughly halfway
// from the neighbour — up to ~9 m off-centre on wide streets, where the
// words are wrong ("Süd-Ost" for what is nearly north). So the announce is
// held until the player is where the words are true:
//   - within a quarter of the cluster's extent of its centre, or
//   - no longer getting closer: passing the closest point, or standing
//     still. A player who never comes near hears it at the best spot the
//     walk offers.
// Left the cluster first: the pending observation is replaced and nothing
// is spoken, as with the dwell gate. One rule for every size, replacing
// the 500 ms dwell and the 1 s Platz delay for these kinds; Großer
// Bereich keeps its old timing (its centre is rarely reached), corridors
// and dead ends keep the dwell (an axis word is true anywhere in a run,
// and a dead end is worth hearing early).
// "Near" uses the cluster's floor footprint, not its node spread, so a
// single-node crossing has a real radius (Taris apartment halls: ~11 m
// wide, 2.8 m radius).
//
// Lead (2026-10-28, first test, Taris Südliche Apartments): firing only
// once the player was no longer getting closer left them 4-5 m past the
// centre — 300 ms to prove the stall, the noun "Kreuzung" (0.42 s), then
// the reaction to let go of W, all at ~4.5 m/s running speed — and out of
// a single-node crossing into the next corridor. What has to happen at
// the centre is the player STOPPING, so the exit words are heard from
// there. A moving player is therefore announced when the closest point of
// their current path lies less than the lead time ahead (or already
// behind): the measured length of the noun plus a reaction allowance.
// Walking slowly fires close to the centre, running fires further out.
// Standing still and slow drifting are left to the stall test.
// The stall numbers only define "no longer getting closer": a step
// toward the centre must gain more than walk jitter, and the stall must
// outlast a single frame hitch. kCentreReactionMs is the one human
// constant: the time from hearing the noun to the key coming up.
constexpr float kCentreRadiusFraction = 0.25f;
constexpr float kCentreProgressEpsM   = 0.05f;
constexpr DWORD kCentreNoCloserMs     = 300;
constexpr DWORD kCentreReactionMs     = 500;
// Time to hear the region noun: "Kreuzung" at the default rate.
constexpr DWORD kCentreLeadMs         = 420;
// Below this the player counts as standing (the stall test's case).
constexpr float kCentreMovingMps      = 0.5f;
extern bool   g_pending_cluster_centre;
extern Vector g_pending_centre_pos;
extern float  g_pending_centre_radius;
extern float  g_pending_centre_best;
extern DWORD  g_pending_centre_best_ms;

// Last spoken room label, used as the text-equality dedup key. The .lyt-
// room partition over-segments KOTOR areas (Endar Spire bridge is split
// into 6 indices that all classify as the same junction); raw room-
// index comparison would re-announce on every micro-crossing. Comparing
// the resolved label collapses the spam: as long as the player walks
// through cells whose label matches, transitions stays quiet. Resets on
// area change.
extern std::string g_last_spoken_room_text;

// Coordinate hysteresis retired 2026-05-13. The original problem it
// solved — 6 announcements in 6 seconds across 5 small .lyt-rooms with
// distinct labels — doesn't exist under Path 3, which collapses
// adjacent same-type regions onto a single nav-graph node. Text-dedup
// alone now handles the only remaining "spam" case (repeated same
// label). With the gate at 4m the system was silencing genuine
// junction-to-junction crossings in dense layouts (Apartments at the
// Dias Apartment cluster, 2026-05-13 patch-20260513-111345.log line
// 11:15:49 "Kreuzung, West, Ost, Nord" silenced by 1.95m displacement).
// The pos-valid/pos fields stay defined for log compatibility but are
// no longer consulted.
extern Vector g_last_spoken_pos;
extern bool   g_last_spoken_pos_valid;

// (A runtime coverage-gap probe fallback lived here briefly in 2026-08 —
// synthetic sentinel ids + a walkmesh shape probe for regions without
// path points. Replaced by build-time walkmesh seeding in room_topology
// (SeedCoverageGapNodes), which gives those regions real nodes, real
// cluster ids and the normal announce path; the parallel machinery was
// removed rather than kept as a dead safety net.)
// Platz delayed-announce state. Since 2026-10-28 only Großer Bereich
// (extent >= the large-area threshold) takes this path; every smaller
// Platz waits for the centre rule instead. When such a cluster
// resolves (kind == KindPlatz) we defer the announce for
// kPlatzDelayMs ms so the player has time to walk further into the
// cluster — by the time the announce fires they're closer to the
// centroid, so the centroid-relative direction list (West, Nord, …)
// matches their actual perception of "which way is which" better.
//
// We re-resolve at fire time: if the player has crossed into a
// different cluster meanwhile, the new label fires instead of the
// stale one.
extern std::string g_pending_platz_text;
extern DWORD  g_pending_platz_tick;
extern bool   g_pending_platz_valid;
extern int    g_pending_platz_room;
extern Vector g_pending_platz_pos;
constexpr DWORD kPlatzDelayMs = 1000;

// Proximity-based landmark fire state. Patch-20260513-045034 showed
// the player walked 1.5m from the "Zu deinem Apartment" waypoint at
// (91.98, 134.42) but never crossed into the .lyt-room that owns
// the landmark — room 11 is a thin sliver around the door. Result:
// the landmark never fired. Decouple landmark firing from .lyt-room
// crossing entirely: each tick, find the nearest landmark within
// enter-range; if stable for N ticks and not the last-spoken one,
// speak it. Exit at a larger range so a landmark re-announces when
// the player walks away and comes back, but not when standing
// nearby for a long time.
//
// Ranges are per-area since 2026-07-17: scaled from the engine's
// fog-of-war cell size so our discovery distance matches the sighted
// map reveal (UpdateMapData explores the party member's cell + 4
// neighbours each tick; a map note pops once its cell is explored —
// see ingame-screens-reference.md fog_of_war_exploration_model). The
// old flat 8m happened to match interiors (Jedi Enclave cell = 7.8m)
// but fell far short on open maps (Dune Sea cell = 44-48m), so the
// player ran past landmarks sighted players had long since seen.
// Computed in RebuildLandmarkCache; floor keeps the historic 8m/12m
// behaviour as the minimum, and the exit range keeps the same 1.5x
// hysteresis ratio.
constexpr float kLandmarkEnterRangeFloorM = 8.0f;
constexpr float kLandmarkFogCellScale     = 1.5f;
constexpr float kLandmarkExitRatio        = 1.5f;
extern float g_lm_enter_range_m;
extern float g_lm_exit_range_m;
constexpr int   kLandmarkStabilityTicks  = 5;
extern int   g_lm_prox_pending_idx;
extern int   g_lm_prox_pending_count;
extern int   g_lm_prox_last_spoken_idx;

// Post-gate cluster re-announce. Cluster changes during combat / blocking
// UI advance state silently; without a refire the player ends combat
// standing in a room they never heard described (Shyrack-Höhle: clusters
// 33/75/76/125/141 all swallowed on first entry). When a gated change is
// recorded, refire the CURRENT room label once the gate has stayed clear
// for kGatedRefireDelayMs — the delay keeps it from talking over the
// combat-end announcement. Text-dedup in SpeakRoomChange makes this a
// no-op when the player is back in the last-spoken room.
constexpr DWORD kGatedRefireDelayMs   = 1500;
extern bool  g_gated_cluster_pending;
extern DWORD g_gate_clear_since_ms;

// Helpers shared across the files, by the file that defines them.

// transitions.cpp
bool IsWorldSpeechGatedImpl();

// transitions_centre.cpp
void TrackMotion(const Vector& pos);
void ResetMotion();
float CentreDistance(const Vector& pos);
bool CentreRuleReady(int clusterId, const Vector& pos);

// transitions_room_speech.cpp
void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos);
void TickPendingPlatz(void* area, const Vector& playerPos);
void TickGatedClusterRefire(void* area, const Vector& playerPos);
void ResetRoomSpeechState();
void ResetPendingPlatzState();
void ResetGatedRefireState();

// transitions_landmarks.cpp
void RebuildLandmarkCache(void* area);
void TickLandmarkCacheRecheck(void* area, const Vector& playerPos);
void TickProximityLandmarks(const Vector& playerPos);
void ResetLandmarkProximityState();

}  // namespace acc::transitions
