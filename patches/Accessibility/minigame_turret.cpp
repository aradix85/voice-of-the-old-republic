// Turret (space-combat gunner) minigame accessibility — see
// turret_game.h for design.
//
// Shares CSWMiniGame with swoop racing; distinguished by type==2 (swoop==1),
// read at CSWMiniGame +0x80. The aim is CSWMiniPlayer.offset (+0x1c4):
// offset.x = elevation°, offset.z = azimuth° — engine-confirmed from the
// decompile of CSWMiniPlayer::Control (it integrates offset and pushes it into
// the rotating gun/camera models each tick). The crosshair is the gun's
// bullethook0 node world direction (ReadAimLine); aim-assist WRITES offset
// (WriteOffset). See docs/llm-docs/turret-minigame-model.md.
//
// What it does:
//   - Entry/exit announce (the native control hint — WASD aims, Space
//     fires; both are native keyboard actions, live-confirmed).
//   - ONE 3D crosshair cue per round, on the SELECTED fighter (Q/E to lock /
//     cycle), using a RISING TICK RATE for centring:
//       * OFF-TARGET — the cue PULSES, panned toward the (lead-corrected)
//         target so the player can swing onto it from anywhere. The pulse
//         interval shrinks as the aim closes on the hit cone (slow pings far
//         off, near-buzz at the edge); behind, the pulses pin to the shorter-
//         swing side.
//       * ON-TARGET — the cue goes SOLID (continuous tone) the instant the aim
//         is inside the range-scaled hitbox. Solid == a shot connects, at any
//         range. (Loudness still swells toward dead-centre within the cone as a
//         secondary cue; it is NOT the primary signal — the engine's distance→
//         gain curve has too steep a knee to track the cone by loudness alone.)
//     Aim point is the INTERCEPT (range-gated lead; see Arena) so distance
//     shots lead the target. Earlier we ran a SECOND ambient engine-loop on the
//     nearest fighters ("hum"); play-testing showed it was mostly ignored, so
//     it was dropped in favour of this single, less-cluttered cue. The vanilla
//     fighters carry no engine loop of their own (CSWTrackFollower+0x144 null);
//     the engine still plays its own one-shot mgs_sith_hit / _expl on
//     hit/kill, which covers "you connected" without us adding anything.
//
// Both games. KOTOR 1 has one turret area (M12ab); KOTOR 2 has three, and
// they differ in bolt speed, fighter size, fighter paths and aim limits (see
// docs/llm-docs/k2-turret-model.md). So the bolt speed / lifespan and the aim
// limits are READ from the engine at entry (the "arena"), and every range the
// cues use is derived from them through a model calibrated on KOTOR 1 — see
// "Range model" below. KOTOR 2's Peragus hangar turret (107PER) has no
// minigame fighters at all: its targets are ordinary area creatures and
// barrels, and its bolts do not travel, so it runs the same cues on an area
// census with the lead switched off ("area-target mode").

#include "minigame_turret_internal.h"

#include "engine_area.h"      // GetCurrentArea + GetClientArea chain,
                              //   AreaObjectIterator + object accessors
#include "log.h"
#include "prism.h"
#include "strings.h"          // Get(TurretGameStarted/Controls/Ended)
#include "engine_game.h"

namespace acc::turret_game {

namespace {

// Turret aim lives in CSWMiniPlayer.offset (+0x1c4) — and the decompile of
// CSWMiniPlayer::Control proves this IS the aim: each tick the engine does
// `offset += axis-velocity; KeepInTunnel(offset)`, then writes offset.x/y/z
// straight into every RotatingModel gun/camera part. So:
//   offset.x = elevation° (W/S axis), offset.z = azimuth° (A/D axis), y unused.
// WASD drives it natively. We aim-assist by WRITING this field (the engine
// re-integrates it next tick, so a per-tick write sticks); see AimAssist below.
// The field address + the read/write primitives live in minigame_aim.h
// (acc::minigame::kMiniPlayerOffsetVectorOffset); here offset.x = elevation,
// offset.z = azimuth.
// The turret minigame type discriminator. swoop is type 1 (swoop_race.cpp);
// anything else on this struct is not ours.
constexpr uint32_t kMiniGameTypeTurret     = 2;

// Exit-debounce: same rationale as swoop_race.cpp — the entry/exit
// transition flips the area chain for a few ticks, so only announce EXIT
// once the latched struct has been gone for this many consecutive ticks.
constexpr int       kExitDebounceTicks     = 60;

// NOTE (RE + measured): sphere_radius is the value OUR cue subtends
// against, but it is NOT the engine's hit primitive. The bullet-hit path is
// CSWMiniGame::DoHitCheck -> CSWTrackFollower::DoBulletHitCheck ->
// CSWMiniGame::HitCheckBullet, which intersects the bullet against the
// follower's MODEL parts (this->models) — mesh geometry, not this scalar.
// None of those three reads sphere_radius, so writing it only widens the cue,
// not the real hitbox. The OnTurretBulletHit diagnostic below CONFIRMED the
// ~20 m hitbox is real and the cue is honest (30 hits across the whole sphere,
// frac to 1.06) — so the cue must NOT be tightened. We then TESTED two ways to
// enlarge the hitbox at runtime — 2x model scale AND sphere_radius=40 — and
// BOTH left the hit radius at ~20 m (the gate is a cached bounding sphere, not
// runtime-patchable). So the hitbox can't be grown here; the easier-to-hit
// lever is aim-assist (magnetism), below. See docs/turret-difficulty-investigation.md.
// ----- TurretHitGeom diagnostic (OnHitBullet hook) -----
// CSWMiniGameObject::OnHitBullet fires once per engine-CONFIRMED bullet hit.
// On the hit-event object (ECX at the hook): +0x54/+0x58/+0x5c hold the
// world-space impact point (the engine feeds them to Play3DOneShotSound), and
// vtable[0x14] returns the victim CSWTrackFollower (the object whose HP it then
// adjusts). |impact - followerCentre| is therefore the distance from centre at
// which the shot actually connected — the real hit radius.
//
// KOTOR 1 only, with the OnPlayerFire shot counter: both hook non-virtual
// functions (CSWMiniGameObject::OnHitBullet, CSWMiniPlayer::Fire) that no
// vtable pairs with a K2 twin — the only K1 callers are the hit check, a
// script-command default handler and a ProcessInput switch case. They feed
// telemetry (TurretHitGeom, the session accuracy line), nothing the player
// hears, so on KOTOR 2 the module runs without them.
const size_t kHitEventImpactXOffset      = acc::off::Kotor1Only(0x54);  // float (world)
const size_t kHitEventImpactYOffset      = acc::off::Kotor1Only(0x58);  // float (world)
const size_t kHitEventImpactZOffset      = acc::off::Kotor1Only(0x5c);  // float (world)
constexpr size_t kVtableSlotGetVictimFollower = 0x14; // OnHitBullet's vtable[0x14]

// ============================================================================
// SEH-guarded primitive reads (same pattern as engine_* / swoop_race).
// ============================================================================

// Read CSWCArea.mini_game via the player-area chain. Source of truth at
// the moment of detection; latched thereafter (the chain churns during
// the entry transition — see swoop_race.cpp).
void* ReadMiniGameViaArea() {
    void* serverArea = acc::engine::GetCurrentArea();
    if (!serverArea) return nullptr;
    void* clientArea = acc::engine::GetClientArea(serverArea);
    return SafeReadPtr(clientArea, kClientAreaMiniGameOffset);
}

// True iff `mg` is a live CSWMiniGame reporting the turret type.
bool IsTurretMiniGame(void* mg) {
    return mg && SafeReadU32(mg, kMiniGameTypeOffset) == kMiniGameTypeTurret;
}

bool LatchedStillValid() {
    if (!g_state.latched_mini_game || !g_state.latched_vtable) return false;
    void* vt = SafeReadPtr(g_state.latched_mini_game, kMiniGameVtableOffset);
    return vt == g_state.latched_vtable;
}

// ============================================================================
// MGO-walk helpers (mirrors swoop_spatial_audio.cpp — kept local so that
// swoop TU isn't entangled with turret-specific cueing).
// ============================================================================

// ============================================================================
// Approach cue. Only the nearest kFighterMaxConcurrent in-range fighters
// loop (the rest are stopped) — see kFighterMaxConcurrent for why all-in-
// range clumped. No forward-only filter: the turret swings a full 360°,
// so a fighter behind still matters. Loops are keyed by MGO slot so each
// follows its fighter across ticks; selection is re-evaluated every tick.
// ============================================================================

void StopAllLoops() {
    g_state.peg_cue.Stop();
    g_state.hum_cue.Stop();
}

// ============================================================================
// Speech. Combine opener + control reminder into ONE interrupting utterance
// so the two halves can't preempt each other (same rationale as the swoop
// opener).
// ============================================================================

void AnnounceEntry() {
    const char* opener   = acc::strings::Get(acc::strings::Id::TurretGameStarted);
    const char* controls = acc::strings::Get(acc::strings::Id::TurretGameControls);
    if ((!opener || !*opener) && (!controls || !*controls)) return;

    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s %s",
                  opener ? opener : "", controls ? controls : "");
    prism::Speak(buf, /*interrupt=*/true);
    acclog::Write("Turret", "spoke entry: [%s]", buf);
}

void AnnounceExit() {
    const char* msg = acc::strings::Get(acc::strings::Id::TurretGameEnded);
    if (msg && *msg) prism::Speak(msg, /*interrupt=*/true);
    acclog::Write("Turret", "spoke exit: [%s]", msg ? msg : "");
}

// ============================================================================
// State transitions.
// ============================================================================

void HandleEnter(void* mg) {
    g_state.active            = true;
    g_state.latched_mini_game = mg;
    g_state.latched_vtable    = SafeReadPtr(mg, kMiniGameVtableOffset);
    g_state.entered_at_ms     = GetTickCount64();
    g_state.ticks_since_lost  = 0;
    g_state.selected_slot     = -1;
    g_state.last_enemy_count  = -1;
    g_state.last_selected_hp  = -1;
    g_state.selected_in_range = -1;
    g_state.selected_on_target = false;
    g_state.selected_behind   = false;
    g_state.last_tick_ms      = 0;
    g_state.leave_tap_due_ms  = 0;
    g_state.have_last_pos     = false;
    g_state.have_vel          = false;
    // Fresh stable-number assignment for this round.
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        g_state.slot_number[i]     = 0;
        g_state.slot_alive_last[i] = false;
        g_state.slot_hp_last[i]    = -1;
        g_state.slot_damaged[i]    = false;
    }
    g_state.next_number       = 1;
    // Fresh aim-quality counters for the session summary.
    g_state.qc_frames = 0;
    g_state.qc_on_total = 0;
    for (int b = 0; b < 3; ++b) { g_state.qc_n[b] = 0; g_state.qc_on[b] = 0; }
    g_state.qc_min_err = 1e9f;
    g_state.qc_sum_err = 0.0f;
    // Fresh aim-assist accuracy telemetry for this round.
    g_state.assist_last_log_ms = 0;
    g_state.shots_fired  = 0;
    g_state.fighter_hits = 0;
    // Fresh aim-assist sign calibration.
    g_state.calib_have      = false;
    g_state.calib_sign_az   = 0; g_state.calib_sign_el = 0;
    g_state.calib_vote_az   = 0; g_state.calib_vote_el = 0;
    // Defensive: no loop from a prior turret session must survive.
    StopAllLoops();

    // Fresh arena (ballistics + aim limits) and area-target tables. A turret
    // whose .are declares no enemies (107PER) shoots at the area instead.
    g_state.arena = Arena{};
    g_state.area_slot_count = 0;
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        g_state.area_handle[i]      = 0;
        g_state.area_is_creature[i] = false;
        g_state.area_noun[i][0]     = '\0';
    }
    uint32_t enemies = SafeReadU32(mg, kMiniGameEnemyCountOffset);
    g_state.area_targets = (enemies == 0);
    acclog::Write("Turret",
                  "ENTER mini_game=%p vtable=%p type=%u enemies=%u%s",
                  mg, g_state.latched_vtable,
                  SafeReadU32(mg, kMiniGameTypeOffset), enemies,
                  g_state.area_targets ? " -> area-target mode" : "");
    ReadArena(mg);

    AnnounceEntry();
}

void HandleExit() {
    ULONGLONG dur = GetTickCount64() - g_state.entered_at_ms;
    acclog::Write("Turret", "EXIT after %llu ms (debounced %d ticks)",
                  dur, kExitDebounceTicks);

    // Session aim-quality summary — the bolt-travel-immune QC metric. "% inside
    // hitbox" by range band measures how well the cue let the player aim,
    // independent of kills (which are noisy: firing cadence, bolt travel, RNG).
    if (g_state.qc_frames > 0) {
        const float pct = 100.0f * g_state.qc_on_total / g_state.qc_frames;
        const float meanErr = g_state.qc_sum_err / g_state.qc_frames;
        auto bandPct = [](int on, int n) { return n > 0 ? 100.0f * on / n : 0.0f; };
        acclog::Write("TurretQC",
                      "session: frames=%d within-hitbox=%d (%.1f%%) "
                      "close=%d/%d (%.0f%%) mid=%d/%d (%.0f%%) far=%d/%d (%.0f%%) "
                      "minErr=%.1f meanErr=%.1f",
                      g_state.qc_frames, g_state.qc_on_total, pct,
                      g_state.qc_on[0], g_state.qc_n[0], bandPct(g_state.qc_on[0], g_state.qc_n[0]),
                      g_state.qc_on[1], g_state.qc_n[1], bandPct(g_state.qc_on[1], g_state.qc_n[1]),
                      g_state.qc_on[2], g_state.qc_n[2], bandPct(g_state.qc_on[2], g_state.qc_n[2]),
                      g_state.qc_min_err, meanErr);
    }

    // Session accuracy telemetry: fighter hits per player shot. Both counters
    // come from the KOTOR-1-only OnTurretBulletHit / OnPlayerFire hooks.
    if (acc::game::IsKotor2()) {
        acclog::Write("TurretAssist", "session accuracy: n/a (fire / hit hooks "
                      "are KOTOR 1 only; see TurretHit lines for hits)");
    } else {
        const float pct = g_state.shots_fired > 0
                              ? 100.0f * g_state.fighter_hits / g_state.shots_fired : 0.0f;
        acclog::Write("TurretAssist", "session accuracy: hits=%d shots=%d (%.1f%%)",
                      g_state.fighter_hits, g_state.shots_fired, pct);
    }

    StopAllLoops();
    g_state.active            = false;
    g_state.latched_mini_game = nullptr;
    g_state.latched_vtable    = nullptr;
    g_state.ticks_since_lost  = 0;
    g_state.selected_slot     = -1;
    g_state.last_enemy_count  = -1;
    AnnounceExit();
}

}  // namespace

// ============================================================================
// TurretHitGeom diagnostic handler — hooked at CSWMiniGameObject::OnHitBullet
// (@0x0066c190), which fires once per engine-CONFIRMED bullet hit. Measures the
// REAL collision radius (|impact - fighterCentre|) against the 20 m
// sphere_radius our cue assumes, to settle whether the cue is honest or
// over-generous. Diagnostic only: reads + logs, no behaviour change.
// ECX = the hit-event object (see the constants block above for its layout).
// ============================================================================
extern "C" void __cdecl OnTurretBulletHit(void* hitEvent) {
    if (!acc::game::HandlerEnabled()) return;  // KOTOR 1 only — no K2 hook (see the hit-event constants)
    if (!hitEvent || !g_state.active) return;

    Vector impact;
    impact.x = SafeReadFloat(hitEvent, kHitEventImpactXOffset);
    impact.y = SafeReadFloat(hitEvent, kHitEventImpactYOffset);
    impact.z = SafeReadFloat(hitEvent, kHitEventImpactZOffset);

    void* follower = CallAsCast(hitEvent, kVtableSlotGetVictimFollower);
    if (!follower) return;

    Vector centre;
    if (!ReadFollowerPosition(follower, centre)) return;

    const float impactDist   = std::sqrt(DistXYZSq(impact, centre));
    const float sphereRadius = SafeReadFloat(follower, kFollowerSphereRadiusOffset);

    // Distinguish our shots landing on a fighter (the hitbox we care about)
    // from incoming fire on the player's own ship. CSWMiniPlayer.follower is
    // the first member, so the player follower pointer == the player pointer.
    void* player = SafeReadPtr(g_state.latched_mini_game, kMiniGamePlayerOffset);

    const bool victimIsPlayer = (follower == player);

    // Accuracy telemetry: count fighter hits (our shots landing).
    if (!victimIsPlayer) ++g_state.fighter_hits;

    acclog::Write("TurretHitGeom",
                  "%s impactDist=%.2fm sphereRadius=%.1f frac=%.2f "
                  "impact=(%.1f,%.1f,%.1f) centre=(%.1f,%.1f,%.1f)",
                  victimIsPlayer ? "PLAYER" : "fighter",
                  impactDist, sphereRadius,
                  (sphereRadius > 0.0f) ? impactDist / sphereRadius : 0.0f,
                  impact.x, impact.y, impact.z,
                  centre.x, centre.y, centre.z);
}

// Player fire counter (hooked at CSWMiniPlayer::Fire @0x0066dc50). One call per
// player fire event; counts PLAYER shots only (enemy fire shares AddBullet, not
// this). Drives the session hits/shots accuracy line. ECX = CSWMiniPlayer (unused).
extern "C" void __cdecl OnPlayerFire(void* /*player*/) {
    if (!acc::game::HandlerEnabled()) return;  // KOTOR 1 only — no K2 hook (see the hit-event constants)
    if (!g_state.active) return;
    ++g_state.shots_fired;
}

bool IsActive() { return g_state.active; }

void Tick() {
    void* mgArea = ReadMiniGameViaArea();

    if (!g_state.active) {
        // Idle. Fire ENTER only for the turret minigame (type==2); swoop
        // (type==1) is handled by swoop_race.cpp.
        if (IsTurretMiniGame(mgArea)) HandleEnter(mgArea);
        return;
    }

    // Active. Verify the latch is still alive (two truth sources — area
    // chain and latched vtable; EXIT only when both say gone for
    // kExitDebounceTicks). Mirrors swoop_race.cpp.
    if (mgArea && mgArea != g_state.latched_mini_game) {
        // Different struct mid-game (engine swap / fresh game). Only
        // re-latch if it's still a turret; otherwise let the debounce
        // run toward EXIT.
        if (IsTurretMiniGame(mgArea)) {
            void* vt = SafeReadPtr(mgArea, kMiniGameVtableOffset);
            acclog::Write("Turret", "re-latch: old=%p new=%p vtable=%p",
                          g_state.latched_mini_game, mgArea, vt);
            g_state.latched_mini_game = mgArea;
            g_state.latched_vtable    = vt;
            g_state.ticks_since_lost  = 0;
        } else {
            ++g_state.ticks_since_lost;
        }
    } else if (mgArea && mgArea == g_state.latched_mini_game) {
        g_state.ticks_since_lost = 0;
    } else if (!mgArea && LatchedStillValid()) {
        // Area chain lost visibility but the struct is intact — stay locked.
        g_state.ticks_since_lost = 0;
    } else {
        ++g_state.ticks_since_lost;
    }

    if (g_state.ticks_since_lost >= kExitDebounceTicks) {
        HandleExit();
        return;
    }

    // Surface kills: when CSWMiniGame.enemy_count drops, a fighter was
    // destroyed (vs. merely leaving cue range, which leaves the count alone).
    const int enemyCount =
        static_cast<int>(SafeReadU32(g_state.latched_mini_game,
                                     kMiniGameEnemyCountOffset));
    if (enemyCount != g_state.last_enemy_count) {
        acclog::Write("Turret", "enemy_count %d -> %d",
                      g_state.last_enemy_count, enemyCount);
        g_state.last_enemy_count = enemyCount;
    }
    // Area-target mode was chosen from the count at entry. Should a space
    // turret have entered before its fighters were loaded, hand back to the
    // fighter census the moment they appear.
    if (g_state.area_targets && enemyCount > 0) {
        g_state.area_targets = false;
        acclog::Write("Turret", "enemies appeared (%d) -> leaving area-target mode",
                      enemyCount);
    }

    // The gun bank / tunnel can still be under construction on the first
    // ticks; keep reading until both halves of the arena are in.
    if (!g_state.arena.ballistics_read || !g_state.arena.limits_read) {
        ReadArena(g_state.latched_mini_game);
    }

    // Aiming (WASD) and firing (Space/Enter) are native keyboard actions.
    // The per-fighter approach loop is the one thing we drive per tick.
    TickFighterCues();
}

}  // namespace acc::turret_game
