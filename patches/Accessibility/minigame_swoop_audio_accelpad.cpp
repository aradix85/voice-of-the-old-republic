// Accelerator pad cues of the swoop race: steering onto the next pad, the
// wall warning, the assist gap and release band, per tick.

#include "minigame_swoop_audio_internal.h"

#include "audio_bus.h"
#include "engine_area.h"      // ReadGlobalNumber: MIN_RACE_GEAR gates the magnet
#include "engine_game.h"
#include "engine_player.h"    // listener: the camera; the engine swaps the
                              //     player creature during the race
#include "log.h"

namespace acc::swoop_race {

namespace {

// ----- Accelerator-pad (CSWMiniEnemy) loop cue -----
//
// Custom resref backed by a trimmed WAV (Override\acc_boost.wav).
// Source: first 0.3 s of mgs_basethrust03 — the engine "base thrust"
// surge with the descending tail removed. The vanilla sample's
// closing half-second slopes down in pitch (reads as the bike
// powering DOWN, opposite of the cue's intent: "boost incoming");
// looping a tail-trimmed version preserves only the rising/sustained
// portion. 0.3 s also fits cleanly inside the per-pad listening
// window (the nearest-only hand-off keeps a cue on each pad ~1-2 s
// before the next pad takes over — see kAccelpadConcurrentLoops).
//
// File ships in Override\ rather than as a renamed engine resource
// so it doesn't collide with any vanilla use of mgs_basethrust03.
// Engine ResLoader checks Override → BIF, so a bare "acc_boost"
// resref resolves to our trimmed sample. (mgs_thrustloop01 is a 5+ s drone
// whose quiet attack is all that plays in the 1-2 s window: inaudible.)
constexpr const char* kAccelpadLoopResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopAccelpadBoost);

// Same 300 m horizon as obstacles (200 m left too little time to settle on
// the pan at gear 3). Reaction time (first-audible lead) at 300 m:
//   gear 1 (max 70 u/s):  ~4.3 s
//   gear 2 (max 120 u/s): ~2.5 s
//   gear 3 (max 190 u/s): ~1.6 s
// NOTE: kForwardCompression is tied to this — it maps this range onto
// the band floor (kSwoopCueMinVolDistM). Change both together.
constexpr float       kAccelpadCueRangeM        = 300.0f;

// ----- Steering guide ------------------------
//
// The guide is a panned acc_boost tone (kAccelpadLoopResref) aimed at the next
// gate, or at the mine / bar detour point when one is active.
//
// Why not the game's own engine hum (the SpaghettiKart design): it is not a
// reachable engine-owned source. The bike's engine loop
// would live at player+0x144 (CSWTrackFollower "Engine" slot, SetSoundName/
// Set3D), BUT the swoop Player's <Engine> resref is EMPTY in all three area
// GFFs (m03/m17/m26 — only the pads carry a Death sound), so SetSoundName makes
// no source and player+0x144 is always null (confirmed live: src=0/playing=-1
// across a whole race). The hum you hear is a
// model-attached sound the engine moves with the bike, not a follower source
// we can address — so we keep panning our own acc_boost tone instead.
// The pads are scattered GATES (they alternate nearly full-lane: 112→81→119→
// 97…), not a smooth racing line — so we aim straight at the next pad's lane,
// not at an interpolated line between pads (that points at the mid-lane, the
// WRONG way).
//
// The tone does not predict around the bike's motion (predicting off a noisy
// velocity pulled the bike off pads it had reached); the velocity brake lives
// in the magnet (kSwoopMagnetBrakeTicks).
//
// EMA on the noisy per-tick velocity. Light smoothing: heavier smoothing's
// ~3-tick lag hurts responsiveness more than the noise does.
constexpr float kSteerVelSmooth  = 0.50f;
constexpr float kSteerVelClamp   = 8.0f;   // reject one-tick bike-X glitches
// Clamp on the steer error (the panned magnitude) and on how far a pad may sit
// laterally from the bike to count as real. The lane is only ~±20 wide, so a
// pad more than 60 u to the side is a junk read (one came back at X=1933 and
// poisoned the smoothed pan to 1695 for ~12 ticks). Both are hard guards.
constexpr float kMaxPadLateral   = 60.0f;

// ----- The assist gap: the ONE difficulty knob ------------------
//
// Accessibility intent (the player's call): keep the SAME challenge a sighted
// player has — the assist must fill only the gap the player cannot close by ear,
// never drive for them. So there is a single lateral tolerance, in lane-units,
// that means BOTH:
//   * the magnet's engage radius — it only pulls once you are within this, and
//   * the cue's release band — the directional cue goes quiet ("you're close
//     enough") here, because this is roughly the precision sound can give you.
// The player steers by ear until inside the gap; the magnet quietly closes the
// rest. That literally "calculates the magnetism into" the cue: you are never
// told to steer for error the magnet will absorb, so steering eases off at the
// end instead of demanding a dead-center read.
//
// ONE knob for difficulty: smaller → the player must steer more precisely by ear
// and fails more (closer to the sighted challenge); larger → the magnet covers
// more. 16u: a logged 28.5 s run had five near-miss gates at 9-14u that the
// player reached by ear but the magnet (weak at its outer edge) couldn't finish. 16u
// pulls those inside reach while the 16-23u medium gates still demand a real
// swing, so challenge stays. Still well inside the ±20 lane. Drop toward ~8 for
// more bite. (The hit itself is still the engine's swept sphere test — this is
// only how much lateral slack the assist forgives.)
constexpr float       kSwoopAssistGapU = 16.0f;
// KOTOR 2 gets its own, smaller gap. Its tracks lay the pads in three fixed
// lanes 11 units apart (Citadel: x -11 / 0 / +11, forward gaps 200-300 u), so
// with the Taris-tuned 16 u the next pad was ALWAYS inside the engage gap the
// moment the previous one was passed: the magnet made every lane change, the
// guide tone rested ~90 % of the race and 23 of 23 pads were caught with the
// player only shifting and jumping (round 5). The magnet also walked the bike into every mine and half-bar on a pad lane. At
// 6 u a one-lane hop needs the player to swing ≥ 5 u toward it by ear before
// the pull engages; the magnet still lands the last stretch (K2's catch
// radius is 4 u — player 2 + pad 2 — against K1's 6). Same knob, per game.
constexpr float       kSwoopAssistGapK2U = 6.0f;

// The on-pad "aligned" blip is centred (no pan), so it needs real forward depth
// to clear the near-field dead zone (cues closer than ~5 m attenuate oddly; 3 m
// was inaudible); 7 m keeps it audible and within range.
constexpr float       kSteerAlignedFwdM = 7.0f;
constexpr const char* kSteerAlignedResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopSteerAligned);
// Base volume (0..127) of the hot synthesised tone (RMS ~44-48%), set by the
// player's request; the global cue slider still scales it.
constexpr uint8_t     kSteerCueVolume = 54;

// Gains sized for the vanilla 5u hit radius: the magnet pulls a bike that reached
// ~8u down to <=5u (about 2.6 u/tick at 8u), an effective window of ~8u.
constexpr float kSwoopMagnetGainFar   = 0.15f;  // pull at the engage edge
constexpr float kSwoopMagnetGainNear  = 0.50f;  // pull dead-on the lane (turret-level)
// Per-tick cap. A per-tick lateral trace measured the PLAYER's own bank at
// ~85 u/s peak (~4.7 u/tick at the ~18 Hz swoop tick), p90 ~46-56 u/s. 5.0 sits
// just ABOVE the player's peak so the magnet can ARREST an overshoot against a
// still-held steer key: last-second misses are overshoots (you reach the pad then
// sail past while holding the key), and below 4.7 the magnet loses that tug-of-war. Contained to near-pad
// overshoot (engages only within the gap; brake only counter-pulls on inward
// velocity), so the player keeps full control elsewhere.
constexpr float kSwoopMagnetMaxStepU  = 5.0f;   // per-tick cap (lane-units)
// Velocity brake (PD derivative): the magnet aims at (padErr − brakeTicks·vel),
// not raw padErr, so when you're closing fast it eases / counter-pulls BEFORE you
// reach the pad — bleeding the overshoot momentum raw position pull can't. Small
// lead (1.5 ticks): enough to anticipate overshoot, not so much you undershoot.
constexpr float kSwoopMagnetBrakeTicks = 1.5f;
// Hold-through-crossing window: within this many seconds of reaching a
// gate's forward position, the pull ramps to fully sticky regardless of lateral
// error, so anticipatory drift toward the next (often opposite-lane) gate can't
// peel the bike off this gate before it scores. Seconds (not units) so the
// window is gear-independent. ~0.35 s ≈ the last ~6 ticks before a crossing.
constexpr float kSwoopMagnetHoldSec   = 0.35f;
// During the pre-race countdown the engine force-centers the bike (speed=0, our
// offset write is clobbered back to lane center), so the magnet would write
// uselessly and spam the heartbeat log. Gate it on the bike actually moving.
// Live: start hold reads speed=0; gear 1 launches to ~9 then 30+ u/s.
constexpr float kSwoopMinRaceSpeed    = 1.0f;   // u/s; below this the race hasn't launched

inline float WallLeadTicks() {
    return acc::game::IsKotor2() ? kWallLeadTicksK2 : kWallLeadTicks;
}

inline float WallEdgeOffsetU() {
    return acc::game::IsKotor2() ? kWallEdgeOffsetK2U : kWallEdgeOffsetU;
}

inline float AssistGapU() {
    return acc::game::IsKotor2() ? kSwoopAssistGapK2U : kSwoopAssistGapU;
}

// ----- Lateral steering magnetism -------
//
// The console-style "sticky aim" from the shipped turret assist, applied to the
// swoop bike's ONE steering axis. The three advisory-cue models (object-in-
// world, ideal-line, co-pilot) all plateaued at ~34% gates because they left
// the PLAYER as the controller of a sub-second full-lane swing; informing
// better can't beat a too-small time budget. Magnetism instead WRITES the
// control field (offset.x at +0x1c4, the bike's lateral lane coord) toward the
// next gate's lane: gentle far, sticky near, capped per tick, BLENDED on top of
// the player's own steering. The cap is well below the player's bank authority
// (±10/tick), so holding a key still overrides — this closes the last units of a
// near-miss, it does not drive the bike. (No full lock-on / autoplay variant —
// that stays turret-only, per the user.)
//
// Grounding: offset.x maps 1:1 to world-X (camera-and-swoop.md), so the
// corrective step is the world lateral error (padErr) directly and the
// offset→world sign is a known +1 (no calibration needed, unlike the turret's
// rotated gun aim). Mid-lane lateral inertia is ~nil (the ±10 bank is not a
// momentum accumulator; real post-release coast ~1 u — see swoop_steering
// notes), so a written offset.x lands cleanly without an inertia model. The
// throttled SwoopAssist log reports offsetX before/after and bikeX.
//
// kSwoopMagnetEngageU — engage only when the next gate is within this lateral
//   error (lane is ±20; gates jump up to ~40 apart). Beyond it, no pull — the
//   player initiates the swing, as with the turret's engage angle. Bound to the
//   shared assist gap, so the magnet is a final-approach gap-filler: the player must close the lane down to the
//   gap by ear, and only then does the magnet quietly finish it. This is THE
//   loosening lever — see kSwoopAssistGapU. (Tune the gap, not this.)
inline float MagnetEngageU() { return AssistGapU(); }  // lane-units; magnetism engages within

}  // namespace

// ============================================================================
// Accelerator-pad STEERING GUIDE — continuous panned tone.
//
// A single looping tone (acc_boost) panned to the NEXT gate's lateral offset, so
// you "drive toward the sound": the pan DIRECTION says which way and the pan
// MAGNITUDE says how far to swing (a slight pan = nudge, a hard pan = full-lane
// gate). Loudness rises as the gate nears, so you also hear urgency / how soon.
// This is the centering paradigm real racing-game a11y mods use (SpaghettiKart /
// "Super Blind Kart"). We pan our OWN tone, not the bike hum — that hum is not a
// reachable engine source (the swoop Player's <Engine> resref is empty in every
// area GFF, so player+0x144 is null; see the constants block above).
//
// Decoupled pan: the cue keeps the pad's true lateral (X) + vertical (Z) but
// pins forward (Y) to a FIXED depth (kSwoopGuidePanForwardRefM), so pan encodes
// lane offset ONLY and is stable — it doesn't lurch as the gate nears.
//
// Rest zone = ~the catch radius (kSwoopGuideSilenceU): the tone goes silent only
// when you're essentially ON the pad, where the magnet finishes the last units.
// It stays present for all real steering (including small nudges) so you never
// "listen for nothing"; it quiets only when you're locked on. Division of labour:
// you do the coarse positioning by ear, the magnet closes the final gap. So the
// player never has to null the pan precisely on the bike (a human-in-the-loop
// that oscillates); coarse magnitude is what pan gives for free.
//
// Pad world positions come from each CSWMiniEnemy's first model via vtable[+0x64]
// (ReadFollowerPosition); pads are reached via the AsEnemy (vtable[0x1c])
// downcast.
// ============================================================================

void TickAccelpadCues(void* miniGame) {
    void* mgoArray = ResolveMgoArray();
    if (!mgoArray) {
        acclog::Trace("SwoopRace", "accelpad cues: mgo array unresolved");
        return;
    }

    Vector listener_pos;
    if (!acc::engine::GetCameraPosition(listener_pos) &&
        !acc::engine::GetPlayerPosition(listener_pos)) {
        acclog::Trace("SwoopRace",
                      "accelpad cues: no listener anchor available");
        return;
    }

    // Bike world position (player follower). The line is evaluated at the
    // bike's own forward Y so the lateral target is "where you should be
    // right now"; the camera (listener) sits a little behind, but its X
    // tracks the bike's to ~0.04 units (verified), so it's the pan anchor.
    void* player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    Vector bikePos;  const bool bikeOk = ReadFollowerPosition(player, bikePos);
    Vector tunnel;   const bool tunOk  =
        SafeReadVector(player, kMiniPlayerOffsetVectorOffset, tunnel);
    const float refY = bikeOk ? bikePos.y : listener_pos.y;
    // Live physics scalars: bike forward speed (gates the magnet off at the start
    // hold) and the player's own sphere_radius (half of the real hit radius).
    const float bikeSpeed   = SafeReadFloat(player, kFollowerSpeedOffset);
    const float playerRadius = SafeReadFloat(player, kFollowerSphereRadiusOffset);

    int slots_seen = 0;
    int accelpads_found = 0;
    int accelpads_ahead = 0;
    int mines_found = 0;
    g_state.mine_count = 0;   // rebuilt every tick for the obstacle sweep

    // The gate to aim at = next pad to cross: the smallest-Y pad strictly ahead
    // whose lateral position is plausible (within kMaxPadLateral of the bike).
    // The outlier guard drops junk reads (one pad came back at X=1933).
    const float bikeXref = bikeOk ? bikePos.x : listener_pos.x;
    int   ahead_slot = -1;  float ahead_y = 0.0f;  Vector ahead_pos = {0,0,0};
    float ahead_radius = 0.0f;  // the nearest gate's sphere_radius (other half of
                                // the real hit radius; all pads share one value).

    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        void* obj = SafeReadPtr(mgoArray,
                                kMgoArrayObjectsOffset +
                                static_cast<size_t>(i) * sizeof(void*));
        if (!obj) continue;
        ++slots_seen;

        void* enemy = CallAsCast(obj, kVtableSlotAsEnemy);
        if (!enemy) continue;

        Vector pos;
        if (!ReadFollowerPosition(enemy, pos)) continue;

        // Pad or mine? Both are CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads +
        // 24 mines, models mgf_acpad / mgf_mine01); treated as pads, mines
        // become steering targets and the bike is driven into them. The model
        // name is the discriminator the track scripts
        // use too (accelpad.ncs looks for "MGM" in the hit object's name).
        void* modelGob = ReadFollowerModelGob(enemy);
        const char* modelName = ReadAurObjectName(modelGob);
        bool isMine = false;
        if (modelName) {
            for (const char* p = modelName; *p; ++p) {
                if ((p[0] == 'm' || p[0] == 'M') && (p[1] == 'i' || p[1] == 'I') &&
                    (p[2] == 'n' || p[2] == 'N') && (p[3] == 'e' || p[3] == 'E')) {
                    isMine = true; break;
                }
            }
        }

        // First-fire diagnostic: every enemy's slot, model and position, so
        // the per-track inventory (and the pad/mine split) is in the log once.
        if (!g_state.accelpad_diag_emitted) {
            void* vt = SafeReadPtr(enemy, 0);
            acclog::Write("SwoopRace",
                          "%s slot=%d ptr=%p vtable=%p model=[%s] "
                          "pos=(%.2f,%.2f,%.2f) radius=%.2f",
                          isMine ? "mine" : "accelpad", i, enemy, vt,
                          modelName ? modelName : "(null)", pos.x, pos.y, pos.z,
                          SafeReadFloat(enemy, kFollowerSphereRadiusOffset));
        }

        if (isMine) {
            ++mines_found;
            if (g_state.mine_count < kMgoArraySlotCount) {
                g_state.mine_slot[g_state.mine_count]   = i;
                g_state.mine_pos[g_state.mine_count]    = pos;
                g_state.mine_radius[g_state.mine_count] =
                    SafeReadFloat(enemy, kFollowerSphereRadiusOffset);
                ++g_state.mine_count;
            }
            continue;   // never a steering target
        }
        ++accelpads_found;

        if (pos.y < refY) continue;                          // behind us
        if (std::fabs(pos.x - bikeXref) > kMaxPadLateral) continue;  // junk read
        ++accelpads_ahead;
        if (ahead_slot < 0 || pos.y < ahead_y) {
            ahead_slot = i; ahead_y = pos.y; ahead_pos = pos;
            ahead_radius = SafeReadFloat(enemy, kFollowerSphereRadiusOffset);
        }
    }

    if (bikeOk && player && acc::game::IsKotor2() && g_state.mine_count > 0) {
        TrackMinePasses(player, bikePos, playerRadius);
    }

    if (!g_state.accelpad_diag_emitted && (accelpads_found + mines_found) > 0) {
        g_state.accelpad_diag_emitted = true;
        acclog::Write("SwoopRace", "enemy pool: accelpads=%d mines=%d",
                      accelpads_found, mines_found);
    }

    // ----- Lateral velocity (units/tick), smoothed + clamped. -----
    if (bikeOk) {
        if (g_state.have_prev_bike_x) {
            float v = bikePos.x - g_state.prev_bike_x;
            if (v >  kSteerVelClamp) v =  kSteerVelClamp;
            if (v < -kSteerVelClamp) v = -kSteerVelClamp;
            g_state.smoothed_vel += (v - g_state.smoothed_vel) * kSteerVelSmooth;
        }
        g_state.prev_bike_x = bikePos.x;
        g_state.have_prev_bike_x = true;
    }

    // ----- Crossing-event log (ground-truth hit scoring). -----
    // When the targeted gate changes, the previous gate has just passed. We have
    // its last in-front sample (prev_ahead_fwdGap > 0, prev_ahead_padErr) and we
    // can recompute its geometry from THIS tick's bike position (fwdGap now <= 0)
    // — so interpolate the exact lateral miss at fwdGap=0. That is the engine's
    // real hit test (|miss| <= ~6 on Taris).
    if (bikeOk && g_state.have_prev_ahead &&
        ahead_slot != g_state.prev_ahead_slot && g_state.prev_ahead_slot >= 0) {
        const float fwdGap_now = g_state.prev_ahead_posY - refY;
        const float padErr_now = g_state.prev_ahead_posX - bikePos.x;
        if (g_state.prev_ahead_fwdGap > 0.0f &&
            fwdGap_now < g_state.prev_ahead_fwdGap) {
            float t = g_state.prev_ahead_fwdGap /
                      (g_state.prev_ahead_fwdGap - fwdGap_now);
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
            const float crossErr =
                g_state.prev_ahead_padErr +
                (padErr_now - g_state.prev_ahead_padErr) * t;
            const float acrx = crossErr < 0.0f ? -crossErr : crossErr;
            // Real hit radius, read live = player.sphere_radius + pad.sphere_radius
            // (the engine's swept sphere-vs-sphere test). All pads share one
            // radius, so the current nearest gate's value scores the just-crossed
            // one. Fall back to the literal only if a read faulted (radius <= 0).
            const float combinedRadius =
                (playerRadius > 0.0f && ahead_radius > 0.0f)
                    ? (playerRadius + ahead_radius)
                    : kAccelpadHitRadiusU;
            const bool caught = acrx <= combinedRadius;
            acclog::Write("SwoopRace",
                          "CROSS slot=%d padX=%.2f crossErr=%.2f abs=%.2f "
                          "hit=%d radius=%.1f vel=%.2f",
                          g_state.prev_ahead_slot, g_state.prev_ahead_posX,
                          crossErr, acrx, caught ? 1 : 0,
                          combinedRadius, g_state.smoothed_vel);
            // Pad caught: one centred burst of the pad's own tone. Neither
            // game's accelpad script plays a sound (both disassembled — speed
            // ×1.05, accel ×1.1, and on KOTOR 2 +100 hit points; the "boost"
            // animation is the only feedback, visual). With the magnet
            // catching every pad (23 of 23 in round 5) the guide tone rests
            // almost the whole race, so by ear nothing
            // said a pad had been scored at all — "I rarely hear an
            // accelpad". Same sample as the guide so the identity is obvious.
            if (caught) acc::audio::PlayCue(kAccelpadLoopResref);
        }
    }

    // padErr/fwdGap are always the TRUE nearest-gate values: the CROSS
    // interpolation relies on them, so the detours below must not move them.
    // ----- Virtual target after the last pad (see kVirtualTargetAheadU):
    // straight ahead of the bike, so the mine-aware line below keeps working
    // to the finish. The magnet is gated off for it unless a detour is active.
    bool virtualTarget = false;
    if (ahead_slot < 0 && bikeOk && acc::game::IsKotor2() && bikeSpeed > kSwoopMinRaceSpeed) {
        virtualTarget = true;
        ahead_pos = bikePos;
        ahead_pos.y += kVirtualTargetAheadU;
    }
    const bool have_target = (ahead_slot >= 0 || virtualTarget) && bikeOk;
    const float padErr = have_target ? (ahead_pos.x - bikePos.x) : 0.0f;
    const float fwdGap = have_target ? (ahead_pos.y - refY) : -1.0f;

    Vector guide_pos = ahead_pos;   // what the tone and the magnet steer to
    float  guideErr  = padErr;     // padErr stays the true pad error

    // ----- Bar-aware line (see kBarFreeClearU and the kPostContactU block).
    // Applied after the mine detour; when both bite the bar wins (a park costs
    // 16-19 s, a mine 100 hp and a downshift). Reads the obstacle sweep's bar
    // cache (with each bar's free-side strip) from last tick.
    int   bar_detour_slot = -1;
    const bool airborne = tunOk && tunnel.z > kAirborneZ;
    if (!airborne) g_state.bar_air_cleared_slot = -1;   // per-flight latch
    if (bikeOk && !airborne) g_state.ground_z = bikePos.z;
    float groundedY = refY;   // where the bike is next on the ground
    float barY      = 0.0f;
    const char* barWhy = "";
    if (have_target && acc::game::IsKotor2() && g_state.bar_count > 0 && fwdGap > 0.0f) {
        const float vWorld = bikeSpeed * kSpeedToWorldPerSec;   // world u/s
        if (airborne) {
            // Remaining flight from the arc: z + vz*t - g*t^2/2 = 0.
            const float vz   = SafeReadFloat(miniGame, kMiniGameJumpVelocityOffset);
            const float disc = vz * vz + 2.0f * kJumpGravity * tunnel.z;
            const float tLand = (vz + std::sqrt(disc > 0.0f ? disc : 0.0f)) / kJumpGravity;
            groundedY = refY + tLand * vWorld;
        }
        float lookAhead = (airborne ? kReJumpSec : kBarDetourLeadSec) * vWorld;
        if (lookAhead < kBarDetourMinLeadU) lookAhead = kBarDetourMinLeadU;
        // Two bars closer than one flight plus a re-jump are a pair: a jump
        // over the first can land in front of the second with no time left.
        const float pairDist = (kJumpOverBarUntilSec + kReJumpSec) * vWorld;
        const bool  parked   = acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") == 0;

        // The nearest bar the bike will meet on the ground inside the
        // look-ahead. The bar already being skirted stays selected until it is
        // behind the bike (the detour moves the line off the span, which would
        // otherwise flip the decision every tick -- the mines' hysteresis), with
        // one exception: a flight that clears it by kBarLandClearU releases it
        // (rule "pull to the pad only when the landing is past the bar");
        // once released in the air it re-engages only if the projection falls
        // clearly short, so it cannot chatter at the threshold.
        int best = -1;
        for (int b = 0; b < g_state.bar_count; ++b) {
            const SpatialAudioState::BarInfo& bar = g_state.bars[b];
            if (bar.y1 + kBarLaneMarginU < refY) continue;             // behind the bike
            const bool sticky = bar.slot == g_state.bar_detour_slot;
            if (airborne) {
                const float clear = (bar.slot == g_state.bar_air_cleared_slot)
                                        ? kBarLaneMarginU : kBarLandClearU;
                if (bar.y1 + clear <= groundedY) {                       // cleared by the flight
                    if (sticky) g_state.bar_air_cleared_slot = bar.slot;
                    continue;
                }
            }
            if (!sticky && bar.y0 > groundedY + lookAhead) continue;    // too far to matter
            if (sticky) { best = b; break; }
            if (best < 0 || bar.y0 < g_state.bars[best].y0) best = b;
        }

        // Which bar's strip, if any: the nearest bar itself, or -- when the
        // nearest is a must-jump (no strip) and a strip bar follows it within
        // pair distance -- that follower, pre-positioned before the jump
        // (MGC05 full, then MGC06 96 u later: be at MGC06's strip before
        // jumping MGC05).
        int target = -1;
        if (best >= 0) {
            const SpatialAudioState::BarInfo& nearest = g_state.bars[best];
            if (nearest.has_gap) {
                target = best;
            } else if (!airborne) {
                for (int b = 0; b < g_state.bar_count; ++b) {
                    const SpatialAudioState::BarInfo& f = g_state.bars[b];
                    if (!f.has_gap || f.y0 <= nearest.y0 || f.y0 - nearest.y0 >= pairDist) continue;
                    if (target < 0 || f.y0 < g_state.bars[target].y0) target = b;
                }
            }
        }
        if (target >= 0) {
            const SpatialAudioState::BarInfo& bar = g_state.bars[target];
            const bool sticky = bar.slot == g_state.bar_detour_slot;
            // Where the straight line to the pad crosses the bar's front edge.
            float t = (bar.y0 - refY) / fwdGap;
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
            const float pathX   = bikePos.x + (ahead_pos.x - bikePos.x) * t;
            const bool  crosses = pathX >= bar.x0 - kBarLaneMarginU &&
                                  pathX <= bar.x1 + kBarLaneMarginU;
            // Pad timing: a pad that lies before the bar is caught first,
            // unless it sits in the span too close in front of the bar to
            // move over afterwards.
            const bool padBehindBar = ahead_pos.y > bar.y0;
            const bool padInSpan    = ahead_pos.x >= bar.x0 - kBarLaneMarginU &&
                                      ahead_pos.x <= bar.x1 + kBarLaneMarginU;
            const bool padTooClose  = !padBehindBar && padInSpan &&
                                      (bar.y0 - ahead_pos.y) < kBarAfterPadLeadSec * vWorld;
            const bool timingOk     = padBehindBar || padTooClose;
            bool engage = false;
            if (sticky || parked)  { engage = true;              barWhy = sticky ? "sticky" : "parked"; }
            else if (airborne)     { engage = crosses;           barWhy = "landing short"; }
            else if (crosses && timingOk) {
                // Ground: only a pair (a bar within pair distance either way)
                // justifies leaving the pad line; an isolated half-bar keeps
                // the beep and the pad behind it.
                bool pair = target != best;   // pre-positioning for a follower
                for (int b = 0; b < g_state.bar_count && !pair; ++b) {
                    const SpatialAudioState::BarInfo& o = g_state.bars[b];
                    if (o.slot == bar.slot) continue;
                    if (std::fabs(o.y0 - bar.y0) < pairDist) pair = true;
                }
                engage = pair;
                barWhy = "pair";
            }
            if (engage) {
                guide_pos.x = bar.gap_x;
                guideErr    = bar.gap_x - bikePos.x;
                bar_detour_slot = bar.slot;
                barY = bar.y0;
            }
        }
    }
    if (bar_detour_slot != g_state.bar_detour_slot) {
        if (bar_detour_slot >= 0) {
            acclog::Write("SwoopRace",
                          "bar detour: slot=%d guideX=%.1f why=%s (pad slot=%d padX=%.1f padY=%.0f "
                          "bikeX=%.1f barY=%.0f airborne=%d groundY=%.0f fwdGap=%.0f)",
                          bar_detour_slot, guide_pos.x, barWhy, ahead_slot, ahead_pos.x,
                          ahead_pos.y, bikePos.x, barY, airborne ? 1 : 0, groundedY, fwdGap);
        } else {
            acclog::Write("SwoopRace", "bar detour: clear (pad slot=%d bikeX=%.1f airborne=%d groundY=%.0f)",
                          ahead_slot, bikePos.x, airborne ? 1 : 0, groundedY);
        }
        g_state.bar_detour_slot = bar_detour_slot;
    }

    // ----- Mine-aware line (see kMineClearU), on the FINAL line: the pad,
    // or the strip the bar-aware block above chose (round 14 lost a race's
    // best 20 s to a strip line that passed a mine at 4 u unchecked).
    int    detour_slot = -1;
    const float line_x   = guide_pos.x;
    const float line_y   = (bar_detour_slot >= 0) ? barY : ahead_pos.y;
    const float lineGap  = have_target ? (line_y - refY) : -1.0f;
    if (have_target && acc::game::IsKotor2() && lineGap > 0.0f) {
        float  bestDy = 1e9f;
        Vector mine   = {0.0f, 0.0f, 0.0f};
        for (int m = 0; m < g_state.mine_count; ++m) {
            const Vector& mpos = g_state.mine_pos[m];
            const float dy = mpos.y - refY;
            if (dy < -kMineBehindU || dy > lineGap) continue;   // behind, or beyond the line's end
            const float t = dy <= 0.0f ? 0.0f : dy / lineGap;
            const float pathX = bikePos.x + (line_x - bikePos.x) * t;
            // Hysteresis: the mine already being detoured stays a detour
            // until it is behind (the detour itself moves the path off it,
            // which flipped the decision every tick in round 10).
            const bool sticky = g_state.mine_slot[m] == g_state.detour_slot;
            if (!sticky && std::fabs(mpos.x - pathX) >= kMineClearU) continue;
            if (dy < bestDy) { bestDy = dy; mine = mpos; detour_slot = g_state.mine_slot[m]; }
        }
        if (detour_slot >= 0) {
            // Pass on the line's side; a mine on the line → toward the centre.
            float side;
            if (std::fabs(line_x - mine.x) < 2.0f) side = (mine.x > 0.0f) ? -1.0f : +1.0f;
            else                                    side = (line_x < mine.x) ? -1.0f : +1.0f;
            float gx = mine.x + side * kMineClearU;
            if (std::fabs(gx) > kMineDetourMaxU) gx = mine.x - side * kMineClearU;
            guide_pos.x = gx;
            guideErr    = gx - bikePos.x;
        }
    }
    if (detour_slot != g_state.detour_slot) {
        if (detour_slot >= 0) {
            acclog::Write("SwoopRace",
                          "detour: mine slot=%d guideX=%.1f (line x=%.1f y=%.0f pad slot=%d padX=%.1f bikeX=%.1f fwdGap=%.0f)",
                          detour_slot, guide_pos.x, line_x, line_y, ahead_slot, ahead_pos.x, bikePos.x, fwdGap);
        } else {
            acclog::Write("SwoopRace", "detour: clear (pad slot=%d)", ahead_slot);
        }
        g_state.detour_slot = detour_slot;
    }

    const ULONGLONG now = GetTickCount64();

    // ----- Lateral steering magnetism (shared aim-assist facility). -----
    // Nudge the bike's lane (offset.x at +0x1c4) toward the next gate's lane:
    // gentle far, sticky near, capped per tick, blended on top of the player's
    // own steering. offset.x maps 1:1 to world-X, so the corrective step is the
    // world lateral error (padErr) directly, sign +1 (no calibration). See the
    // constants block for the engine grounding.
    // `tunnel` IS the offset Vector (+0x1c4) read above; we replace only its x.
    // Gear gate: no pull in gear 0. After a KOTOR 2 collision the bike sits in
    // gear 0 in front of whatever it hit, and the first K2 round showed the
    // magnet dragging every relaunch back into the same bar (bike steered to
    // x=-10, magnet returned it to x=0 within the relaunch second, hit again).
    // The player must be free to steer clear before the pull resumes; the
    // gear global is 0 exactly then (and before the first launch, where the
    // speed gate already held). Both games' scripts maintain the global.
    const bool inGear = acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") >= 1;
    if (have_target && bikeOk && tunOk && player && inGear &&
        (!virtualTarget || detour_slot >= 0) &&
        bikeSpeed > kSwoopMinRaceSpeed &&
        std::fabs(guideErr) <= MagnetEngageU()) {
        const acc::minigame::MagnetParams mp{
            kSwoopMagnetGainFar, kSwoopMagnetGainNear, kSwoopMagnetMaxStepU};
        float t = 1.0f - std::fabs(guideErr) / MagnetEngageU();
        if (t < 0.0f) t = 0.0f;
        // Hold-through-crossing: in the last kSwoopMagnetHoldSec before
        // this gate's forward position is reached, ramp the pull to fully sticky
        // (t->1) regardless of lateral error, so anticipatory drift toward the
        // NEXT gate can't peel the bike off this one before it scores. Speed-
        // scaled (time-to-cross) so the window is the same in every gear. Targets
        // the "dead-on then drifted out" + "engaged but didn't finish" misses
        // (22 of 31 misses in one traced run).
        if (fwdGap > 0.0f && bikeSpeed > 1.0f) {
            const float ttc = fwdGap / bikeSpeed;                  // seconds to crossing
            if (ttc < kSwoopMagnetHoldSec) {
                const float hold = 1.0f - ttc / kSwoopMagnetHoldSec;  // 0 far -> 1 at crossing
                t += (1.0f - t) * hold;
            }
        }
        const float gain = acc::minigame::MagnetGain(t, mp);
        // Velocity brake: aim at where you'll coast to (padErr − brakeTicks·vel),
        // not the raw pad, so when you're closing fast this goes negative before
        // you arrive and the magnet counter-pulls, bleeding overshoot momentum.
        // smoothed_vel is bike-X velocity (+ = right); padErr = padX − bikeX (+ =
        // pad to the right), so subtracting the velocity lead damps inward motion
        // on either side. mappedErr = −(target error).
        const float brakedErr = guideErr - kSwoopMagnetBrakeTicks * g_state.smoothed_vel;
        const float newOffX =
            acc::minigame::MagnetStep(tunnel.x, -brakedErr, gain, mp);
        Vector newOff = tunnel;
        newOff.x = newOffX;
        acc::minigame::WriteOffsetVector(player, newOff);

        if (now - g_state.last_magnet_log_ms >= 1000) {
            g_state.last_magnet_log_ms = now;
            acclog::Write("SwoopAssist",
                          "magnet slot=%d padErr=%.2f vel=%.2f brakedErr=%.2f "
                          "gain=%.2f offX %.2f->%.2f bikeX=%.2f padX=%.2f fwdGap=%.1f",
                          ahead_slot, padErr, g_state.smoothed_vel, brakedErr,
                          gain, tunnel.x, newOffX,
                          bikePos.x, ahead_pos.x, fwdGap);
        }
    }

    // ----- Predictive overshoot / wall cue. -----
    // Fire the wall-impact sound ~kWallLeadTicks before the bike pins instead of
    // on contact, so re-steering starts sooner. Guard: only on a genuine
    // OVERSHOOT — projected to cross the lane edge AND moving away from the
    // target pad (vel and padErr opposite signs) — so it stays silent when you're
    // correctly racing toward a pad that sits near the edge. tunnel.x is the lane
    // offset (±kWallEdgeOffsetU = wall); smoothed_vel is the lateral velocity
    // (world-X delta, same units as the offset delta).
    if (have_target && tunOk) {
        const float vel       = g_state.smoothed_vel;
        const float projected = tunnel.x + vel * WallLeadTicks();
        const bool  toward_wall  = std::fabs(projected) >= WallEdgeOffsetU() &&
                                   std::fabs(tunnel.x)  <  WallEdgeOffsetU();
        const bool  overshooting = vel * padErr < 0.0f;  // moving away from pad
        if (toward_wall && overshooting && std::fabs(vel) > kWallMinVel &&
            now - g_state.last_wall_cue_ms >= kWallCueDebounceMs) {
            const int side = (vel > 0.0f) ? +1 : -1;
            Vector cue = listener_pos;
            cue.x += (side > 0) ? +kWallCuePanM : -kWallCuePanM;
            acc::audio::PlayCue3DRaw(kWallCueResref, cue);
            g_state.last_wall_cue_ms = now;
            acclog::Write("SwoopRace",
                          "wall predict side=%s tunnelX=%.1f vel=%.2f "
                          "proj=%.1f padErr=%.1f",
                          side > 0 ? "right" : "left", tunnel.x, vel,
                          projected, padErr);
        }
    }

    // ----- KOTOR 2 wall loop (see kWallBandU). Band OR projected crossing,
    // any gear, with hysteresis so it does not chatter at the band edge.
    if (acc::game::IsKotor2() && tunOk) {
        // The projection counts only on a true overshoot — moving AWAY from
        // the guide target (or with no target). Round 9 showed it firing on
        // every fast swing toward a side-lane pad (proj crosses 16 from x 6
        // while the magnet is about to catch the pad at 11), which trains
        // the ear to ignore it. The band itself is unconditional.
        const float vel  = g_state.smoothed_vel;
        const float proj = tunnel.x + vel * WallLeadTicks();
        const bool  overshoot = !have_target || vel * guideErr < 0.0f;
        // The band is unconditional — except while the guide itself asks for
        // it: a half-bar's strip beside the posts (MGC03 at x −12.4, MGC06 at
        // −13.1) would otherwise ride under a permanent wall thump. The
        // projected post crossing still warns there.
        const bool guideInBand = have_target && bar_detour_slot >= 0 &&
                                 std::fabs(guide_pos.x) >= kWallBandU - kWallBandHystU;
        const bool projOk = overshoot && (!guideInBand || std::fabs(vel) >= kWallLoopBandMinVel);
        int side = 0;
        if ((!guideInBand && tunnel.x >= kWallBandU) || (projOk && proj >= kWallEdgeOffsetK2U))       side = +1;
        else if ((!guideInBand && tunnel.x <= -kWallBandU) || (projOk && proj <= -kWallEdgeOffsetK2U)) side = -1;
        if (side == 0 && !guideInBand && g_state.wall_loop_side != 0 &&
            tunnel.x * g_state.wall_loop_side > kWallBandU - kWallBandHystU) {
            side = g_state.wall_loop_side;   // still in the hysteresis band
        }
        if (side != 0) {
            Vector cue = listener_pos;
            cue.x += side * kWallLoopPanM;
            cue.y += kWallLoopFwdM;
            if (g_state.wall_loop.IsActive()) {
                g_state.wall_loop.UpdatePosition(cue);
            } else {
                g_state.wall_loop.Start(
                    kWallCueResref, cue, /*looping=*/true, /*spatial=*/true,
                    /*priorityGroup=*/-1, /*volumeByte=*/kSwoopVolNearByte,
                    /*maxVolDist=*/kSwoopFlatBandMaxM,
                    /*minVolDist=*/kSwoopFlatBandMinM);
            }
            if (side != g_state.wall_loop_side) {
                acclog::Write("SwoopRace",
                              "wall loop: side=%s tunnelX=%.1f vel=%.2f proj=%.1f gear=%d overshoot=%d",
                              side > 0 ? "right" : "left", tunnel.x,
                              vel, proj, inGear ? 1 : 0, overshoot ? 1 : 0);
            }
        } else if (g_state.wall_loop.IsActive()) {
            g_state.wall_loop.Stop();
            acclog::Write("SwoopRace", "wall loop: stop tunnelX=%.1f", tunnel.x);
        }
        g_state.wall_loop_side = side;
    }

    // ----- Continuous panned steering guide (see the function header). -----
    // Pan = next gate's lateral offset (decoupled: true X/Z, forward pinned to a
    // fixed depth → pan encodes lane offset only). Loudness = real approach
    // distance (urgency), floored so it stays audible. Rests (silent) only when
    // you're within ~the catch radius of the pad — the magnet finishes from there.
    bool guide_active = false;
    // Rest radius: the pad's catch radius for a pad, but a strip target is
    // only 5 u past the bar's end, so resting 5 u short of it leaves the bike
    // still touching the bar with nothing to hear — round 17 sat 2.5 u off
    // the MGC04 strip, inside the span, with "follow the tone" spoken and the
    // tone silent.
    const float restU = (bar_detour_slot >= 0) ? kBarStripRestU : kSwoopGuideSilenceU;
    if (have_target && bikeOk && std::fabs(guideErr) > restU) {
        const float dist = std::sqrt(DistXYZSq(guide_pos, listener_pos));
        Vector cue_pos = guide_pos;
        cue_pos.y = listener_pos.y + kSwoopGuidePanForwardRefM;
        int vol_byte = SwoopVolumeByte(dist, kAccelpadCueRangeM);
        if (vol_byte < kSwoopGuideMinVolByte) vol_byte = kSwoopGuideMinVolByte;
        if (g_state.accelpad_line_loop.IsActive()) {
            g_state.accelpad_line_loop.UpdatePosition(cue_pos);
            g_state.accelpad_line_loop.UpdateVolume(vol_byte);
        } else {
            g_state.accelpad_line_loop.Start(
                kAccelpadLoopResref, cue_pos, /*looping=*/true, /*spatial=*/true,
                /*priorityGroup=*/-1, /*volumeByte=*/vol_byte,
                /*maxVolDist=*/kSwoopFlatBandMaxM,
                /*minVolDist=*/kSwoopFlatBandMinM);
        }
        guide_active = true;
    }
    if (!guide_active && g_state.accelpad_line_loop.IsActive()) {
        g_state.accelpad_line_loop.Stop();  // within the rest zone / no target
    }

    // On-pad confirmation: when the tone just went from sounding (steering) to
    // rest (you reached the catch radius), play one centred "aligned" blip —
    // "you're on track, you can stop steering." The panned tone's silence alone
    // was ambiguous (off? blended out? actually locked?); this makes the rest
    // state a positive cue. Fires once per approach (on the transition), re-arms
    // when you steer off again.
    if (have_target && g_state.guide_was_active && !guide_active) {
        Vector cue = listener_pos;                      // centred — no pan
        cue.y = listener_pos.y + kSteerAlignedFwdM;
        acc::audio::PlayCue3DRaw(kSteerAlignedResref, cue,
                              /*priorityGroup=*/0, kSteerCueVolume);
        acclog::Trace("SwoopRace", "guide aligned: padErr=%.2f slot=%d",
                      padErr, ahead_slot);
    }
    g_state.guide_was_active = guide_active;

    // ----- Steering-guide diagnostic -----
    // guide=1 when the panned tone is sounding, 0 when it rests (on the pad /
    // no target).
    if (have_target) {
        acclog::Trace("SwoopRace",
                      "guide: padX=%.2f bikeX=%.2f padErr=%.2f vel=%.2f "
                      "guide=%d fwdGap=%.1f tunnelX=%s%.2f",
                      ahead_pos.x, bikePos.x, padErr, g_state.smoothed_vel,
                      guide_active ? 1 : 0, fwdGap,
                      tunOk ? "" : "FAULT:", tunOk ? tunnel.x : 0.0f);
    }

    acclog::Trace("SwoopRace",
                  "accelpad scan: slots=%d accelpads=%d ahead=%d aheadSlot=%d "
                  "refY=%.1f",
                  slots_seen, accelpads_found, accelpads_ahead, ahead_slot,
                  refY);

    // Remember this tick's targeted gate for next tick's crossing interpolation.
    if (have_target) {
        g_state.prev_ahead_slot   = ahead_slot;
        g_state.prev_ahead_posX   = ahead_pos.x;
        g_state.prev_ahead_posY   = ahead_pos.y;
        g_state.prev_ahead_padErr = padErr;
        g_state.prev_ahead_fwdGap = fwdGap;
        g_state.have_prev_ahead   = true;
    } else {
        g_state.have_prev_ahead   = false;
        g_state.prev_ahead_slot   = -1;
    }
}

}  // namespace acc::swoop_race
