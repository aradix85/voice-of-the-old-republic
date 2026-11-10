// Area and region changes, spoken: the per-tick tracker that notices a new
// area or a new room_topology cluster, waits until the change is stable,
// and hands it to the room-speech and landmark parts of the module.

#include "transitions_internal.h"

#include "bringup_announce.h" // IsMoviePlaying — gate speech during movies
#include "combat.h"          // IsCombatActive — gate room-change speech
#include "discovery.h"       // discovery-tier area reconciliation + landmark record
#include "endar_softlock.h"  // Endar Spire room-5 softlock guard
#include "engine_area.h"
#include "exit_memory.h"     // destination / visited / way-back words on exits
#include "engine_input.h"    // ForceReacquireInput — post-load DirectInput re-grab
#include "engine_panels.h"   // IsForegroundUiBlocking — gate vs. menus / dialogs
#include "engine_player.h"
#include "log.h"
#include "narrated_target.h" // clear on area transition
#include "same_name_suffix.h"   // Reset() on area transition
#include "strings.h"
#include "speech.h"
#include "room_topology.h"      // single source of truth for perceptual-
                                // region labels (nav-graph decomposition)

namespace acc::transitions {

namespace {

// The area last seen; nullptr until the first, so the first tick with an
// area announces it: the orientation line on game load.
void* g_prev_area = nullptr;

// Last friendly room name observed at the player position. Tracked so
// we ALSO fire on friendly-name change inside a single cluster — covers
// the TSL case where two adjacent .lyt-rooms share a cluster but have
// distinct human-readable names. K1 never trips this path in vanilla
// content (every room name is resref-style and filtered out by
// IsResrefStyleRoomName); the cost is a strcmp per tick.
char g_prev_friendly_room_name[64] = {0};

// Stability dedup for the cluster trigger. Cluster boundaries flicker
// far less than .lyt-room boundaries (the player needs ~2-3m of motion
// to cross a real cluster boundary vs ~0.3m for some rooms), but a
// short stability window still smooths the rare snap-flip at a
// boundary. ~80ms at 60fps — too short to feel laggy, long enough to
// absorb a single-tick boundary glitch.
constexpr int kClusterStabilityTicks = 5;
int g_pending_cluster_id    = acc::room_topology::kClusterIdNone;
int g_pending_cluster_count = 0;

// The pending cluster waits for the centre rule (transitions_centre.cpp).
bool g_pending_cluster_centre = false;

// Minor-cluster dwell gate. Outdoor organic walkmesh (the rock paths of
// the Lehon beaches) fragments into clusters of 3-8 m, crossed in one to
// three seconds. A region smaller than the player's stride is noise
// mid-stride, and crossing it resets the current region, so re-entering
// the big neighbour would re-announce. So a cluster below
// kMinorClusterExtentM announces only after the player has STAYED in it
// for kMinorClusterDwellMs; passing through keeps the previous region
// committed. Friendly-name changes bypass the gate. It covers corridors
// and dead ends (and the rare exit-less open fallback); clusters whose
// exit words are measured from a centre take the centre rule instead.
//
// 500 ms: a single-node junction has extent 0 and a ~2 m snap footprint,
// and a longer dwell swallowed junctions crossed in stride. Hearing a label
// cut off by the next is what lets the player turn back; a missed crossing
// costs far more than a clipped one. If outdoor fragment noise returns,
// fix the clustering rather than lengthen the gate.
constexpr float kMinorClusterExtentM = 12.0f;
constexpr DWORD kMinorClusterDwellMs = 500;
DWORD g_pending_cluster_since_ms = 0;
bool  g_pending_cluster_minor    = false;

// Everything the module remembers about the current area. Called on every
// area change, and when the player is lost (a load), so the next area
// starts clean either way.
void ResetAreaState() {
    g_prev_cluster_id            = acc::room_topology::kClusterIdNone;
    g_pending_cluster_id         = acc::room_topology::kClusterIdNone;
    g_pending_cluster_count      = 0;
    g_pending_cluster_centre     = false;
    g_prev_friendly_room_name[0] = '\0';
    ResetMotion();
    ResetRoomSpeechState();
    ResetLandmarkProximityState();
}

void SpeakArea(void* area) {
    char nameBuf[128] = {0};
    if (!acc::engine::GetAreaDisplayName(area, nameBuf, sizeof(nameBuf)) ||
        nameBuf[0] == '\0') {
        // Even when name resolution fails entirely, log the event so
        // post-mortem can correlate the silence against an area change.
        acclog::Write("Transition", "area change detected but name resolve failed; "
            "areaPtr=%p", area);
        return;
    }
    char speech[160] = {0};
    std::snprintf(speech, sizeof(speech),
                  acc::strings::Get(acc::strings::Id::FmtTransitionArea),
                  nameBuf);
    acc::speech::Speak(speech, /*interrupt=*/false);
    acclog::Write("Transition", "area -> '%s' (areaPtr=%p)", nameBuf, area);
}

}  // namespace

void Tick() {
    // Stay completely silent + hands-off while an engine movie owns the
    // foreground (intro logos, stunt cutscene movies like 03.bik at the
    // Leviathan capture). KOTOR's movie player aborts its play queue if
    // windows/focus churn during playback, which surfaces as the game
    // closing instead of starting the next queued movie. We touch NO state
    // and don't advance g_prev_area, so the area-change branch below re-fires
    // cleanly on the first tick after the movie ends: the area announce is
    // deferred, not dropped. The main loop is usually frozen during playback anyway;
    // this guards the boundary ticks where it isn't.
    if (acc::bringup_announce::IsMoviePlaying()) {
        return;
    }

    Vector pos = {};
    if (!acc::engine::GetPlayerPosition(pos)) {
        // Reset state on player loss so the next in-game tick re-anchors
        // cleanly (matches camera_announce's reset-on-gate-failure
        // discipline).
        g_prev_area = nullptr;
        ResetAreaState();
        acc::room_topology::Reset();
        acc::narration::Reset();
        // Door numbering classes are frozen per area visit; drop the old
        // area's snapshot so the next narration rebuilds from the new one.
        acc::engine::ResetDoorLockSnapshot();
        // Leave g_module_load_pending alone. Player-loss is the standard
        // mid-load symptom (PC slot wiped while the engine swaps modules);
        // clearing the latch here would re-open the per-tick probes
        // immediately and undo the whole point of the latch. The flag
        // clears on the area-pointer change below, which marks the new
        // module as actually surfaced.
        return;
    }

    void* area = acc::engine::GetCurrentArea();
    if (!area) {
        // Player is loaded but area resolve faulted (mid-load hand-off).
        // Don't reset prev_area — the area pointer is stable across the
        // brief windows where GetCurrentArea returns null mid-frame, and
        // resetting would re-fire the area announce next tick.
        return;
    }

    if (area != g_prev_area) {
        // Lift the module-load latch. The engine has surfaced a new area
        // pointer, so the CResRef-arena swap that the latch was protecting
        // against is complete. Per-tick consumers can resume probing
        // engine accessors on player / leader / area-object state.
        g_module_load_pending = false;
        // Re-acquire DirectInput now that the load is fully complete. The
        // engine recreates its render window during a load; if the player
        // pressed keys during that churn the input `active` flag can desync
        // to 1 while the keyboard is actually unacquired, and the engine's
        // own post-load SetActive(1) no-ops against the stuck flag — leaving
        // the keyboard dead until an alt-tab. We reach this edge AFTER the
        // engine's HideLoadScreen has run, so forcing a 0->1 cycle here lands
        // last and wins. Harmless on a healthy load (a sub-frame device
        // re-grab, exactly what the engine does on alt-tab). See
        // engine_input.h ForceReacquireInput for the full mechanism.
        acc::engine::ForceReacquireInput();
        SpeakArea(area);
        // Drop the unified narrated-target slot — any object from the old
        // area is now invalid. Stale pointers would survive otherwise
        // (TryGet's handle round-trip resolves through the active area's
        // game-object array, so a cross-area stale obj could in principle
        // get a false-positive validation). Explicit Clear is the safe
        // path.
        acc::narrated_target::Clear();
        // Reconcile the discovery-tier index for the new area: capture its
        // tag and clear the in-memory set. The actual read of the save var is
        // deferred (discovery::Tick) until the player creature's table has
        // settled — a read here, on the save-load tick, can race
        // CSWSObject::LoadObjectState. On a normal walk-through transition the
        // creature isn't reloaded, so this is effectively just a re-key.
        acc::discovery::OnAreaChanged(area);
        acc::endar::OnAreaChanged(area);   // Endar Spire softlock guard re-key
        // Rebuild the per-room landmark cache for the new area before
        // any room-change branch can fire — the first room announce
        // after an area change should already use the curated label
        // when one exists.
        RebuildLandmarkCache(area);
        // Build the nav-graph region decomposition for the new area.
        // Single source of truth shared with the map cursor; built once
        // on area-enter so walking and map-cursor exploration see the
        // same labels.
        //
        // The wall-edge cache the decomposition depends on is built by
        // spatial::wall_surfaces, which runs EARLIER in the dispatch
        // order (core_tick.cpp: wall_surfaces then transitions —
        // deliberately, so we never build a graph from the previous
        // area's walls). It can still come back empty on this exact
        // tick when the engine has not made the area's walkmesh data
        // available yet, and BuildForArea silently leaves the graph
        // empty in that case, so we retry below on every tick until it
        // builds successfully. core_tick.cpp's dispatch-order notes are
        // authoritative.
        // Re-arm the nav-graph retry before building: this is the only
        // genuine "we entered an area" edge, so an area previously
        // abandoned as unreadable gets a fresh set of attempts here (and
        // only here). Ordering matters — BuildForArea consults the latch.
        acc::room_topology::ResetNavGraphRetry();
        acc::room_topology::BuildForArea(area);
        acc::exit_memory::OnAreaEntered(area);
        g_prev_area = area;
        ResetAreaState();
    } else if (!acc::room_topology::HasGraphForArea(area)) {
        // Same area as last tick but the graph still isn't built —
        // wall cache wasn't ready when the area-change branch fired.
        // Retry cheaply each tick until it builds. BuildForArea
        // self-gates on the wall cache (no-op until populated) and
        // is idempotent on a same-area call once built.
        acc::room_topology::BuildForArea(area);
    }

    // Door snapshot stabiliser. The initial SnapshotDoors call inside
    // BuildForArea can lose late-arriving door handles to a partially-
    // populated server-object array; this per-tick refresh keeps
    // re-snapshotting until the count settles. No-op once the door
    // set commits or until the graph itself is built.
    acc::room_topology::MaybeRefreshDoors(area);

    // Landmarks, every tick and before the region gates below, so standing
    // still in one region does not stop them. The staleness recheck runs
    // first, so a note the engine just enabled is proximity-eligible on the
    // same tick the player is already standing next to it.
    TickLandmarkCacheRecheck(area, pos);

    TickProximityLandmarks(pos);

    // Every tick, so the velocity is current the moment a cluster becomes
    // pending (the centre rule's lead).
    TrackMotion(pos);

    // The delayed place line fires whenever its delay elapses, outside the
    // stability gate below, so it reliably lands ~1 s after entering.
    TickPendingPlaza(area, pos);

    // Post-combat room refire: if a cluster change was gated during the
    // fight, describe where the player actually ended up once speech is
    // allowed again.
    TickGatedClusterRefire(area, pos);

    // Cluster-based trigger. We probe room_topology at the player position
    // each tick: the canonical "what perceptual region am I in" signal,
    // stable across .lyt-room flicker.
    //
    // Sentinel handling:
    //   kClusterIdNone     — graph not built yet / outside snap radius
    //                        with no candidates. Don't trigger; keep
    //                        prev state so the next valid cluster
    //                        compares correctly.
    //   kClusterIdOpenArea — LookupAt's synthesised open-area fallback.
    //                        A STABLE id, so leaving labelled terrain
    //                        fires exactly one announcement and
    //                        re-entering a labelled cluster fires again.
    //
    // We ALSO track friendly room-name change as a secondary trigger so
    // TSL content (where two .lyt-rooms inside one cluster can have
    // distinct human-readable names) still announces on the name flip.
    // K1 vanilla content never trips this — every room name is resref-
    // style and IsResrefStyleRoomName filters it out.
    std::string shapeBuf;
    int  shapeSig      = 0;
    int  clusterId     = acc::room_topology::kClusterIdNone;
    // Per-tick probe: suppress LookupAt's WALL-FILTERED / ALL-BLOCKED
    // diagnostic logs. Speech-time call sites (SpeakRoomChange peek,
    // ResolveRoomSpeech, LogWallTopoComparison) still emit them on
    // every cluster transition, so we keep full visibility into wall-
    // filter behaviour without flooding the log with identical lines
    // at 60 fps when the player stands at a wall-filtered boundary.
    acc::room_topology::LookupAt(area, pos,
                                 shapeBuf,
                                 shapeSig, clusterId,
                                 /*allowDiagLog=*/false,
                                 /*requireWallReachable=*/true,
                                 /*zTrusted=*/true);

    // Resolve the friendly room name (if any) for the player's current
    // .lyt-room. Empty / resref-style names normalise to "".
    char friendlyBuf[64] = {0};
    {
        int roomIdx = -1;
        acc::engine::GetRoomAtIndexed(area, pos, roomIdx);
        if (roomIdx >= 0) {
            char nameBuf[64] = {0};
            if (acc::engine::GetRoomDisplayName(area, roomIdx,
                                                nameBuf, sizeof(nameBuf)) &&
                nameBuf[0] != '\0' &&
                !IsResrefStyleRoomName(nameBuf)) {
                strncpy_s(friendlyBuf, nameBuf, _TRUNCATE);
            }
        }
    }

    // Nothing to compare against yet — wait for the next tick where the
    // graph has finished building. Don't advance prev state on kClusterIdNone
    // so the first real cluster observation still triggers.
    if (clusterId == acc::room_topology::kClusterIdNone &&
        friendlyBuf[0] == '\0') {
        return;
    }

    // Snap-gap seam suppression. kClusterIdOpenArea is the synthetic
    // fallback LookupAt returns when the player sits beyond the snap
    // radius of every cluster node. In an area that already has labelled
    // clusters that is almost never "genuinely open" — it's a thin
    // coverage seam *between* two regions (the Kashyyyk Great Walkway has
    // a ~15 m band between cluster 4 and cluster 12 where neither node is
    // in range). Announcing the bare fallback there and re-acquiring the
    // real cluster on the far side produces boundary thrash.
    // Hold the last real cluster across the seam by collapsing the
    // fallback to the previous id, so it reads as "still in that region."
    // A genuinely open area still announces the fallback once: there the
    // previous committed id is None/-2 (no real cluster was ever in
    // range), so this guard doesn't fire.
    if (clusterId == acc::room_topology::kClusterIdOpenArea &&
        g_prev_cluster_id >= 0) {
        clusterId = g_prev_cluster_id;
    }

    // The doorway sliver of an area door (the lift just arrived by) is a
    // region of its own, joined to the rest only through that door. Track
    // and announce the region in front of the door instead, with its label
    // resolved at that region's centre (exit_memory::TransitionSliverOwner).
    Vector labelPos       = pos;
    bool   sliverRedirect = false;
    {
        Vector ownerCentre{};
        const int owner = acc::exit_memory::TransitionSliverOwner(
            area, clusterId, &ownerCentre);
        if (owner != clusterId) {
            clusterId      = owner;
            labelPos       = ownerCentre;
            sliverRedirect = true;
        }
    }

    bool clusterChanged  = (clusterId != g_prev_cluster_id);
    bool friendlyChanged = (std::strncmp(friendlyBuf,
                                         g_prev_friendly_room_name,
                                         sizeof(g_prev_friendly_room_name)) != 0);

    if (!clusterChanged && !friendlyChanged) {
        // Stable inside the same cluster + same friendly name — clear
        // any pending different-cluster observation (player wandered
        // toward the boundary then back).
        g_pending_cluster_id    = acc::room_topology::kClusterIdNone;
        g_pending_cluster_count = 0;
        return;
    }

    // A change is observed. Require kClusterStabilityTicks consecutive
    // observations of the SAME new cluster before announcing — filters
    // single-tick boundary glitches. We key the stability on cluster_id
    // alone (friendly-name changes inside the same cluster fire after
    // the same window because the friendly check happens on commit).
    if (clusterId == g_pending_cluster_id) {
        ++g_pending_cluster_count;
        // Stood here for the stability window: visited, even if this region
        // never commits (minor regions passed through, centre rule unmet).
        if (g_pending_cluster_count == kClusterStabilityTicks) {
            acc::exit_memory::OnClusterSeen(clusterId);
        }
    } else {
        g_pending_cluster_id       = clusterId;
        g_pending_cluster_count    = 1;
        g_pending_cluster_since_ms = GetTickCount();
        // Classify once on first observation — labels are baked per
        // build, so the verdict can't change while pending.
        g_pending_cluster_minor  = false;
        g_pending_cluster_centre = false;
        acc::room_topology::ClusterInfo info;
        if (acc::room_topology::GetClusterInfo(area, clusterId, info) &&
            info.extentM >= 0.0f) {
            const bool centreKind =
                info.kind == acc::room_topology::KindJunction ||
                (!info.largeArea &&
                 (info.kind == acc::room_topology::KindRoom ||
                  info.kind == acc::room_topology::KindPlaza));
            if (centreKind) {
                g_pending_cluster_centre = true;
                ArmCentreRule(clusterId, info, pos, g_pending_cluster_since_ms);
            } else if (info.extentM < kMinorClusterExtentM) {
                g_pending_cluster_minor = true;
                acclog::Write("Transition",
                              "cluster -> %d is minor (extent=%.1fm < %.1fm, "
                              "kind=%d) — dwell-gated %lums",
                              clusterId, info.extentM, kMinorClusterExtentM,
                              info.kind, (unsigned long)kMinorClusterDwellMs);
            }
        }
    }

    // From the doorway sliver the player is not walking toward the region's
    // centre yet; neither the centre rule nor the minor-region dwell apply.
    if (sliverRedirect) {
        g_pending_cluster_centre = false;
        g_pending_cluster_minor  = false;
    }

    // Track the closest approach every tick from the first observation,
    // not only once the stability window has passed.
    if (g_pending_cluster_centre && !friendlyChanged) {
        // Not at the spot the label's words are measured from yet: hold.
        // Leaving first never commits, same as the dwell gate.
        if (!CentreRuleReady(clusterId, pos)) return;
        g_pending_cluster_centre = false;  // met: latch until committed
    }

    if (g_pending_cluster_count >= kClusterStabilityTicks &&
        g_pending_cluster_minor && !friendlyChanged &&
        (GetTickCount() - g_pending_cluster_since_ms) < kMinorClusterDwellMs) {
        // Sub-perceptual cluster and the player hasn't lingered yet:
        // hold. Passing through never commits, so the previous region
        // stays current and won't re-announce on return.
        return;
    }

    if (g_pending_cluster_count >= kClusterStabilityTicks) {
        // Gate speech (not state) on combat / blocking-UI. We still
        // commit g_prev_cluster_id so the player walking back-and-forth
        // during combat doesn't queue up a burst of announcements for
        // the moment combat ends.
        // Exit memory follows the committed region whether or not the
        // announce is gated, so "visited" and "back" stay true.
        acc::exit_memory::OnClusterCommitted(g_prev_cluster_id, clusterId);
        if (IsWorldSpeechGated()) {
            acclog::Write("Transition",
                          "cluster -> %d gated (combat / blocking UI), "
                          "state advanced silently", clusterId);
            NoteGatedClusterChange();
        } else {
            SpeakRoomChange(area, clusterId, labelPos);
        }
        g_prev_cluster_id = clusterId;
        strncpy_s(g_prev_friendly_room_name, friendlyBuf, _TRUNCATE);
        g_pending_cluster_id    = acc::room_topology::kClusterIdNone;
        g_pending_cluster_count = 0;
    }
}

bool IsWorldSpeechGated() {
    acc::engine::UiBlockState ui;
    return acc::combat::IsCombatActive() ||
           acc::engine::IsForegroundUiBlocking(&ui);
}

}  // namespace acc::transitions
