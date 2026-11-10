// What a region change says: the region label, the flap guard against a
// label spoken twice on a boundary, the delayed place line, and the refire
// of a change that was gated while speech was blocked.

#include "transitions_internal.h"

#include "engine_area.h"
#include "exit_memory.h"     // destination / visited / way-back words on exits
#include "log.h"
#include "menus_modsettings.h"  // RoomShapes toggle gates the shape tier
#include "speech.h"
#include "room_topology.h"      // single source of truth for perceptual-
                                // region labels (nav-graph decomposition)

namespace acc::transitions {

namespace {

// Boundary flap dedup (2026-07-15, Shyrack-Höhle). A small-catchment
// cluster next to a large one flips the nearest-node winner with half
// a step near the seam: the log shows 'Nord-Süd' ↔ 'Nord-West, Süd'
// alternating four times in 16 s at one spot, and a Kreuzung ↔ Bereich
// pair re-announcing 11 s apart from the same walkmesh room. Suppress
// only the RETURN leg: a label is skipped when it was itself spoken
// within kFlapWindowMs AND the player is still within kFlapRadiusM of
// where they heard it. First-time sequences through distinct regions
// (Korridor → Kreuzung → Bereich) stay fast and untouched, and a real
// revisit (walked away and back) re-announces. Unlike the retired
// 2026-05-13 coordinate hysteresis this can never silence a NEW label —
// it only ever eats the exact label heard seconds ago at the same spot.
constexpr DWORD kFlapWindowMs = 15000;
constexpr float kFlapRadiusM  = 4.0f;
// Label spoken one before the current g_last_spoken_room_text, with
// where/when it was spoken.
std::string g_flap_prev_text;
Vector      g_flap_prev_pos    = {0.0f, 0.0f, 0.0f};
DWORD       g_flap_prev_ms     = 0;
// Where/when the CURRENT g_last_spoken_room_text was spoken (rotates
// into the prev slot on the next commit).
Vector      g_flap_cur_pos     = {0.0f, 0.0f, 0.0f};
DWORD       g_flap_cur_ms      = 0;

// True when speaking `text` at `pos` would be the flap return-leg.
bool IsFlapRepeat(const std::string& text, const Vector& pos, DWORD now) {
    if (text.empty() || text != g_flap_prev_text) return false;
    if ((now - g_flap_prev_ms) >= kFlapWindowMs)  return false;
    return DistXYSq(pos, g_flap_prev_pos) < (kFlapRadiusM * kFlapRadiusM);
}

// Rotate the two-deep spoken-label history. Call wherever a label is
// committed as spoken (or Platz-enqueued, which commits the text).
void CommitSpokenLabel(const std::string& text, const Vector& pos,
                       DWORD now) {
    g_flap_prev_text = g_last_spoken_room_text;
    g_flap_prev_pos  = g_flap_cur_pos;
    g_flap_prev_ms   = g_flap_cur_ms;
    g_flap_cur_pos   = pos;
    g_flap_cur_ms    = now;
    g_last_spoken_room_text = text;
}

// Resolve the speech at `worldPos` using a two-tier order:
//   1. Friendly room name at the resolved .lyt-room (filters resref-
//      style ids) → "room_name"
//   2. room_topology::LookupAt (nav-graph topology) → "shape"
//
// Tier 1 stays on .lyt-room ids: the friendly-name table is keyed by
// room index, and the room-name look-up itself is stable for the rare
// authored names. Tier 2 is cluster-aware via room_topology. Returns
// false (and leaves outBuf empty) when no tier resolves. Vanilla
// resref-style rooms with no shape classification stay silent rather
// than announce a meaningless engine-internal id.
//
// Transition / doorway composition was removed 2026-05-13 after
// empirical evidence (Oberstadt 40m east-west street labelled
// "Türschwelle" 14 times in a row) showed K1's .lyt-room boundaries
// don't correspond to doorways. The Path 3 classifier now treats every
// degree-2 node as a corridor; real doorway detection would need
// wall-geometry constriction sensing.
// outExitsOpt receives the shape label's exits for exit_memory (empty for a
// friendly room name).
bool ResolveRoomSpeech(void* area, const Vector& worldPos,
                       std::string& outBuf,
                       const char*& outSource,
                       int* outClusterIdOpt = nullptr,
                       std::vector<acc::room_topology::ExitMark>* outExitsOpt = nullptr) {
    outBuf.clear();
    if (outExitsOpt) outExitsOpt->clear();
    outSource = "none";
    if (outClusterIdOpt) *outClusterIdOpt = acc::room_topology::kClusterIdNone;

    // Landmark tier removed from the room-transition path 2026-05-13.
    // Landmarks now fire via TickProximityLandmarks below, which scans
    // every waypoint each tick and triggers on geometric proximity to
    // the waypoint position — not on .lyt-room crossing.

    int roomIndex = -1;
    acc::engine::GetRoomAtIndexed(area, worldPos, roomIndex);

    char tier1Buf[128] = {0};
    if (roomIndex >= 0 &&
        acc::engine::GetRoomDisplayName(area, roomIndex,
                                        tier1Buf, sizeof(tier1Buf)) &&
        tier1Buf[0] != '\0' &&
        !IsResrefStyleRoomName(tier1Buf)) {
        outBuf = tier1Buf;
        outSource = "room_name";
        // Still consult cluster for the trigger key — fire on either
        // friendly-name OR cluster change.
        if (outClusterIdOpt) {
            std::string scratch;
            int  scratchSig = 0;
            acc::room_topology::LookupAt(area, worldPos,
                                         scratch,
                                         scratchSig, *outClusterIdOpt,
                                         /*allowDiagLog=*/true,
                                         /*requireWallReachable=*/true,
                                         /*zTrusted=*/true);
        }
        return true;
    }

    // Mod Settings → Room shape descriptions: when OFF, skip the
    // room_topology shape tier entirely. Friendly room names (tier 1)
    // still announce because they're authored content, not synthesised
    // corridor / junction / Platz vocabulary. Cluster-id stays at None
    // so SpeakRoomChange's caller logs "unresolved" and the Platz
    // delay-enqueue path never arms.
    if (!acc::menus::modsettings::GetToggle(
            acc::menus::modsettings::Option::RoomShapes)) {
        return false;
    }

    std::string path3Buf;
    int  path3Sig    = 0;
    int  clusterId   = acc::room_topology::kClusterIdNone;
    if (acc::room_topology::LookupAt(area, worldPos,
                                     path3Buf,
                                     path3Sig, clusterId,
                                     /*allowDiagLog=*/true,
                                     /*requireWallReachable=*/true,
                                     /*zTrusted=*/true,
                                     outExitsOpt) &&
        !path3Buf.empty()) {
        outBuf = path3Buf;
        outSource = "shape";
        if (outClusterIdOpt) *outClusterIdOpt = clusterId;
        return true;
    }
    return false;
}

// Diagnostic — log room_topology's resolution at this position alongside
// whatever the resolved speech was (room name, shape, or transition
// compose). Mirrors the announcement quad we see in MapCursor / ViewMode
// so a single grep across categories shows every nav-graph decision.
void LogWallTopoComparison(void* area, int roomIndex,
                           const Vector& worldPos,
                           const char* spoken,
                           const char* source) {
    std::string path3;
    int  path3Sig    = 0;
    int  path3Cid    = acc::room_topology::kClusterIdNone;
    bool havePath3 = acc::room_topology::LookupAt(
        area, worldPos, path3, path3Sig, path3Cid,
        /*allowDiagLog=*/true,
        /*requireWallReachable=*/true,
        /*zTrusted=*/true);
    const int path3Kind = havePath3 ? (path3Sig & 0xff) : -1;

    acclog::Write("WallTopo.Compare",
                  "pos=(%.2f,%.2f) room=%d spoken=\"%s\" src=%s | "
                  "path3=\"%s\" kind=%d sig=%d cluster=%d",
                  worldPos.x, worldPos.y, roomIndex,
                  spoken ? spoken : "", source ? source : "",
                  havePath3 ? path3.c_str() : "<no graph>",
                  path3Kind, path3Sig, path3Cid);
}

// Speak a perceptual-region change. The caller (Tick) decides whether
// to invoke us based on cluster_id transitions; this function resolves
// the label, applies text-equality dedup against `g_last_spoken_room_text`
// as a belt-and-braces filter (rare case: two clusters generating the
// exact same label string), and speaks. Gated by the caller via
// combat / UI-blocking checks.
//
// Platz delay: when the resolved label is a multi-node Path 3 cluster
// (kind == KindPlatz) of Großer-Bereich size, defer the announce for
// kPlatzDelayMs (smaller ones take the centre rule in Tick instead). The
// label + position are stashed; `TickPendingPlatz` fires the announce
// later, re-resolving at the current player position so a player who
// walked through doesn't get a stale label.
//
// `clusterId` is the cluster the player just entered, captured by the
// caller via LookupAt. We log it for diagnostics; resolution itself
// goes through ResolveRoomSpeech so the friendly-name tier wins when
// available (TSL coverage).
// The player's own region announce: exit_memory adds what lies behind each
// exit, whether it was visited, and which one leads back. Falls back to the
// plain label when there is nothing to add. View mode keeps calling
// SpeakRegionLabel directly — these words are about the player's path.
void SpeakPlayerRegionLabel(void* area, const std::string& label,
                            const std::vector<acc::room_topology::ExitMark>& exits) {
    std::string annotated;
    const bool added = acc::exit_memory::Annotate(area, label, exits, annotated);
    acc::transitions::SpeakRegionLabel(added ? annotated.c_str() : label.c_str());
}

}  // namespace

void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos) {
    std::string speechBuf;
    std::vector<acc::room_topology::ExitMark> exits;
    const char* source = "none";
    int resolvedCluster = acc::room_topology::kClusterIdNone;
    if (!ResolveRoomSpeech(area, worldPos,
                           speechBuf, source,
                           &resolvedCluster, &exits) ||
        speechBuf.empty()) {
        acclog::Write("Transition",
                      "cluster -> %d unresolved (src=none) — staying silent "
                      "(areaPtr=%p)", clusterId, area);
        return;
    }

    // Resolve a diagnostic roomIndex once for log + LogWallTopoComparison.
    // Not used to gate anything — the trigger is upstream on cluster_id.
    int roomIndex = -1;
    acc::engine::GetRoomAtIndexed(area, worldPos, roomIndex);

    if (speechBuf == g_last_spoken_room_text) {
        acclog::Write("Transition",
                      "cluster -> %d '%s' src=%s room=%d — text-dedup "
                      "match, silent",
                      clusterId, speechBuf.c_str(), source, roomIndex);
        return;
    }

    // Boundary flap: this exact label was spoken moments ago and the
    // player hasn't really left the spot — the return leg of a seam
    // ping-pong, not new information. History is NOT rotated on the
    // suppressed leg, so the pair stays quiet until real displacement
    // or the window expiring re-arms it.
    if (IsFlapRepeat(speechBuf, worldPos, GetTickCount())) {
        acclog::Write("Transition",
                      "cluster -> %d '%s' src=%s room=%d — flap-dedup "
                      "(spoken %lums ago within %.1fm), silent",
                      clusterId, speechBuf.c_str(), source, roomIndex,
                      (unsigned long)(GetTickCount() - g_flap_prev_ms),
                      kFlapRadiusM);
        return;
    }

    // Peek at the Path 3 kind to decide between immediate announce and
    // Platz-delay path. Re-runs LookupAt — cheap (linear scan over ~50
    // nodes); avoids threading the sig back through ResolveRoomSpeech.
    std::string peekBuf;
    int    peekSig      = 0;
    int    peekCid      = acc::room_topology::kClusterIdNone;
    int    peekKind     = -1;
    if (acc::room_topology::LookupAt(area, worldPos,
                                     peekBuf,
                                     peekSig, peekCid,
                                     /*allowDiagLog=*/true,
                                     /*requireWallReachable=*/true,
                                     /*zTrusted=*/true)) {
        peekKind = peekSig & 0xff;
    }
    // Only Großer Bereich still defers: a smaller Platz waited for the
    // centre rule in Tick, so the player is already where its words hold.
    bool peekLarge = false;
    if (peekKind == acc::room_topology::KindPlatz) {
        acc::room_topology::ClusterInfo info;
        if (acc::room_topology::GetClusterInfo(area, peekCid, info))
            peekLarge = info.largeArea;
    }

    if (peekKind == acc::room_topology::KindPlatz && peekLarge) {
        // Defer: stash the resolved label + context for the timer in
        // Tick to fire. Advance g_last_spoken_room_text now so further
        // cluster transitions producing the same label get text-deduped
        // out — we only fire once per Platz entry.
        g_pending_platz_text  = speechBuf;
        g_pending_platz_tick  = GetTickCount();
        g_pending_platz_room  = roomIndex;
        g_pending_platz_pos   = worldPos;
        g_pending_platz_valid = true;
        CommitSpokenLabel(speechBuf, worldPos, g_pending_platz_tick);
        acclog::Write("Transition",
                      "cluster -> %d '%s' src=%s room=%d — Platz, deferred "
                      "%lums (areaPtr=%p)",
                      clusterId, speechBuf.c_str(), source, roomIndex,
                      (unsigned long)kPlatzDelayMs, area);
        LogWallTopoComparison(area, roomIndex, worldPos, speechBuf.c_str(),
                              source);
        return;
    }

    // Non-Platz path. Cancel any pending Platz announce (we just entered
    // a different shape, the deferred Platz no longer applies).
    g_pending_platz_valid = false;

    SpeakPlayerRegionLabel(area, speechBuf, exits);
    CommitSpokenLabel(speechBuf, worldPos, GetTickCount());
    g_last_spoken_pos       = worldPos;
    g_last_spoken_pos_valid = true;
    acclog::Write("Transition",
                  "cluster -> %d '%s' src=%s room=%d (areaPtr=%p)",
                  clusterId, speechBuf.c_str(), source, roomIndex, area);
    LogWallTopoComparison(area, roomIndex, worldPos, speechBuf.c_str(), source);
}

// Fire any pending Platz announce whose delay has elapsed. Re-resolves
// the label at the player's CURRENT position so a player who walked
// through the cluster in <1s gets the label they actually ended up at
// (or no announce, if they're now somewhere with the same label as the
// pending one — that already got committed to last_spoken_room_text on
// enqueue, so text-dedup eats the re-fire).
void TickPendingPlatz(void* area, const Vector& playerPos) {
    if (!g_pending_platz_valid) return;
    DWORD now = GetTickCount();
    if ((now - g_pending_platz_tick) < kPlatzDelayMs) return;

    // Diagnostic roomIdx for the log; not used for resolution itself.
    int roomIdx = -1;
    acc::engine::GetRoomAtIndexed(area, playerPos, roomIdx);

    std::string fireBuf;
    std::vector<acc::room_topology::ExitMark> exits;
    const char* source = "none";
    // Still in the doorway sliver of an area door: the pending label belongs
    // to the region in front of it and was resolved at that region's centre
    // (TickCluster); resolving at the player would read the sliver again.
    Vector resolvePos = playerPos;
    {
        std::string sliverLabel;
        int sliverSig = 0;
        int sliverCid = acc::room_topology::kClusterIdNone;
        if (acc::room_topology::LookupAt(area, playerPos, sliverLabel,
                                         sliverSig, sliverCid,
                                         /*allowDiagLog=*/false,
                                         /*requireWallReachable=*/true,
                                         /*zTrusted=*/true) &&
            acc::exit_memory::TransitionSliverOwner(area, sliverCid,
                                                    nullptr) != sliverCid) {
            resolvePos = g_pending_platz_pos;
        }
    }
    bool resolved = ResolveRoomSpeech(area, resolvePos,
                                      fireBuf, source,
                                      nullptr, &exits) &&
                    !fireBuf.empty();

    if (resolved && fireBuf == g_pending_platz_text) {
        // Same Platz still under the player. Fire the announce now.
        SpeakPlayerRegionLabel(area, fireBuf, exits);
        g_last_spoken_pos       = playerPos;
        g_last_spoken_pos_valid = true;
        acclog::Write("Transition",
                      "Platz announce fired after %lums: '%s' (room=%d "
                      "areaPtr=%p)",
                      (unsigned long)(now - g_pending_platz_tick),
                      fireBuf.c_str(), roomIdx, area);
    } else if (resolved && fireBuf != g_last_spoken_room_text) {
        // Player has moved on to a different shape during the delay.
        // Fire the NEW label so we don't leave them unannounced.
        SpeakPlayerRegionLabel(area, fireBuf, exits);
        CommitSpokenLabel(fireBuf, playerPos, now);
        g_last_spoken_pos       = playerPos;
        g_last_spoken_pos_valid = true;
        acclog::Write("Transition",
                      "Platz announce superseded after %lums: pending='%s' "
                      "current='%s' src=%s (room=%d areaPtr=%p)",
                      (unsigned long)(now - g_pending_platz_tick),
                      g_pending_platz_text.c_str(), fireBuf.c_str(), source,
                      roomIdx, area);
    } else {
        // Either resolution failed or text-dedups against the same
        // last-spoken label (we already advanced last_spoken to the
        // pending Platz label on enqueue, so this branch fires when
        // the player is back where they started — same label).
        acclog::Write("Transition",
                      "Platz announce expired (no new label) pending='%s' "
                      "(room=%d areaPtr=%p)",
                      g_pending_platz_text.c_str(), roomIdx, area);
    }
    g_pending_platz_valid = false;
}

// Refire the current room label after a gated cluster change, once the
// gate (combat / blocking UI) has stayed clear for kGatedRefireDelayMs.
// Resolution happens at the CURRENT player position — if several gated
// changes piled up during a fight, only where the player actually ended
// up gets spoken. SpeakRoomChange's text-dedup keeps this silent when
// the player is back in the last room they already heard.
void TickGatedClusterRefire(void* area, const Vector& playerPos) {
    if (!g_gated_cluster_pending) return;

    DWORD now = GetTickCount();
    if (IsWorldSpeechGatedImpl()) {
        g_gate_clear_since_ms = 0;  // gate re-engaged — restart the clock
        return;
    }
    if (g_gate_clear_since_ms == 0) {
        g_gate_clear_since_ms = now;
        return;
    }
    if ((now - g_gate_clear_since_ms) < kGatedRefireDelayMs) return;

    g_gated_cluster_pending = false;
    g_gate_clear_since_ms   = 0;
    acclog::Write("Transition",
                  "gated-cluster refire: gate clear for %lums — "
                  "re-resolving at current position",
                  (unsigned long)kGatedRefireDelayMs);
    SpeakRoomChange(area, g_prev_cluster_id, playerPos);
}

bool IsResrefStyleRoomName(const char* name) {
    if (!name || name[0] == '\0') return true;
    if ((name[0] == 'm' || name[0] == 'M') && name[1] >= '0' && name[1] <= '9') {
        return true;
    }
    // KOTOR 2 rooms are "<module number><suffix>" — "106per10", "103perd".
    // A leading digit never opens an authored display name, so it is as
    // strong a noise signal as K1's m+digit. Without this, the first K2
    // session with working room resolution spoke "103perd" as an area
    // label (2026-08-05).
    if (name[0] >= '0' && name[0] <= '9') return true;
    if ((name[0] == 's' || name[0] == 'S') &&
        (name[1] == 't') && (name[2] == 'u') &&
        (name[3] == 'n') && (name[4] == 't')) {
        return true;
    }
    for (const char* p = name; *p; ++p) {
        if (*p == '_') return true;
    }
    return false;
}

// --- Per-group area-change resets ---------------------------------------
//
// Tick() used to clear all of this inline, seventeen assignments reaching
// into four unrelated state groups. That is what made the state hard to
// reason about (and what defeated an attempt to split this file): no
// group owned its own lifecycle, so nothing could move without dragging
// the reset code with it.
//
// Each group now clears itself. Adding a variable to a group means
// updating exactly one function, next to the variables it clears.
// Behaviour is identical — same variables, same values, same order.

// Cluster / room-label speech, including the text-equality dedup key and
// the flap-suppression window.
void ResetRoomSpeechState() {
    g_prev_cluster_id            = acc::room_topology::kClusterIdNone;
    g_pending_cluster_id         = acc::room_topology::kClusterIdNone;
    g_pending_cluster_count      = 0;
    g_pending_cluster_centre     = false;
    ResetMotion();
    g_prev_friendly_room_name[0] = '\0';
    g_last_spoken_room_text.clear();
    g_last_spoken_pos_valid      = false;
    g_flap_prev_text.clear();
    g_flap_prev_ms               = 0;
    g_flap_cur_ms                = 0;
}

// Deferred "Platz" announcement.
void ResetPendingPlatzState() {
    g_pending_platz_valid = false;
}

// Post-gate cluster re-announce (combat / blocking UI swallowed a change).
void ResetGatedRefireState() {
    g_gated_cluster_pending = false;
    g_gate_clear_since_ms   = 0;
}

void SpeakRegionLabel(const char* text) {
    acc::speech::Speak(text, /*interrupt=*/true);
}

}  // namespace acc::transitions
