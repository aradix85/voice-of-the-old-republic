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

namespace {

// One tick of the pad cues: the readings, the nearest pad ahead, and the line
// the guide tone and the magnet steer to.
struct PadTick {
    Vector    listener = {0.0f, 0.0f, 0.0f};
    void*     player = nullptr;
    Vector    bikePos = {0.0f, 0.0f, 0.0f};
    bool      bikeOk = false;
    Vector    tunnel = {0.0f, 0.0f, 0.0f};  // the bike's offset Vector (+0x1c4)
    bool      tunOk = false;
    float     refY = 0.0f;
    float     bikeSpeed = 0.0f;
    float     playerRadius = 0.0f;
    ULONGLONG now = 0;

    int       slotsSeen = 0;
    int       padsFound = 0;
    int       padsAhead = 0;
    int       minesFound = 0;
    int       aheadSlot = -1;
    Vector    aheadPos = {0.0f, 0.0f, 0.0f};
    float     aheadRadius = 0.0f;  // all pads share one sphere_radius

    bool      virtualTarget = false;
    bool      haveTarget = false;
    // padErr and fwdGap stay the true nearest-pad values (the crossing score
    // relies on them); the detours move only guidePos and guideErr.
    float     padErr = 0.0f;
    float     fwdGap = -1.0f;
    Vector    guidePos = {0.0f, 0.0f, 0.0f};
    float     guideErr = 0.0f;
    int       barDetourSlot = -1;
    float     barY = 0.0f;
    int       mineDetourSlot = -1;
    bool      inGear = false;
};

// The listener (the camera, else the player) and the bike's readings. The
// camera trails the bike, but its X tracks the bike's to ~0.04 units, so it is
// the pan anchor; the line is evaluated at the bike's own forward Y.
bool ReadTickInputs(void* miniGame, PadTick& t) {
    if (!acc::engine::GetCameraPosition(t.listener) &&
        !acc::engine::GetPlayerPosition(t.listener)) {
        acclog::Trace("SwoopRace",
                      "accelpad cues: no listener anchor available");
        return false;
    }
    t.player = SafeReadPtr(miniGame, kMiniGamePlayerOffset);
    t.bikeOk = ReadFollowerPosition(t.player, t.bikePos);
    t.tunOk  = SafeReadVector(t.player, kMiniPlayerOffsetVectorOffset, t.tunnel);
    t.refY   = t.bikeOk ? t.bikePos.y : t.listener.y;
    // Forward speed gates the magnet at the start hold; sphere_radius is the
    // player's half of the real hit radius.
    t.bikeSpeed    = SafeReadFloat(t.player, kFollowerSpeedOffset);
    t.playerRadius = SafeReadFloat(t.player, kFollowerSphereRadiusOffset);
    return true;
}

// Pads and mines are both CSWMiniEnemy on KOTOR 2 (211TEL: 23 pads, 24 mines);
// the model name tells them apart (mgf_acpad / mgf_mine01), as in the track
// scripts.
bool IsMineModel(const char* name) {
    if (!name) return false;
    for (const char* p = name; *p; ++p) {
        if ((p[0] == 'm' || p[0] == 'M') && (p[1] == 'i' || p[1] == 'I') &&
            (p[2] == 'n' || p[2] == 'N') && (p[3] == 'e' || p[3] == 'E')) {
            return true;
        }
    }
    return false;
}

// Walk the enemy pool: cache the mines for the obstacle sweep, and find the
// gate to aim at, the nearest pad strictly ahead whose lateral position is
// plausible (one junk read put a pad at X=1933).
void ScanEnemies(void* mgoArray, PadTick& t) {
    g_state.mine_count = 0;   // rebuilt every tick for the obstacle sweep
    const float bikeXref = t.bikeOk ? t.bikePos.x : t.listener.x;
    for (int i = 0; i < kMgoArraySlotCount; ++i) {
        void* obj = SafeReadPtr(mgoArray,
                                kMgoArrayObjectsOffset +
                                static_cast<size_t>(i) * sizeof(void*));
        if (!obj) continue;
        ++t.slotsSeen;

        void* enemy = CallAsCast(obj, kVtableSlotAsEnemy);
        if (!enemy) continue;

        Vector pos;
        if (!ReadFollowerPosition(enemy, pos)) continue;

        const char* modelName = ReadAurObjectName(ReadFollowerModelGob(enemy));
        const bool isMine = IsMineModel(modelName);

        // First fire: every enemy once in the log, for the track inventory.
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
            ++t.minesFound;
            if (g_state.mine_count < kMgoArraySlotCount) {
                g_state.mine_slot[g_state.mine_count]   = i;
                g_state.mine_pos[g_state.mine_count]    = pos;
                g_state.mine_radius[g_state.mine_count] =
                    SafeReadFloat(enemy, kFollowerSphereRadiusOffset);
                ++g_state.mine_count;
            }
            continue;   // never a steering target
        }
        ++t.padsFound;

        if (pos.y < t.refY) continue;                                  // behind us
        if (std::fabs(pos.x - bikeXref) > kMaxPadLateral) continue;    // junk read
        ++t.padsAhead;
        if (t.aheadSlot < 0 || pos.y < t.aheadPos.y) {
            t.aheadSlot   = i;
            t.aheadPos    = pos;
            t.aheadRadius = SafeReadFloat(enemy, kFollowerSphereRadiusOffset);
        }
    }

    if (t.bikeOk && t.player && acc::game::IsKotor2() && g_state.mine_count > 0) {
        TrackMinePasses(t.player, t.bikePos, t.playerRadius);
    }

    if (!g_state.accelpad_diag_emitted && (t.padsFound + t.minesFound) > 0) {
        g_state.accelpad_diag_emitted = true;
        acclog::Write("SwoopRace", "enemy pool: accelpads=%d mines=%d",
                      t.padsFound, t.minesFound);
    }
}

// Lateral velocity in units per tick, clamped and smoothed.
void UpdateLateralVelocity(const PadTick& t) {
    if (!t.bikeOk) return;
    if (g_state.have_prev_bike_x) {
        float v = t.bikePos.x - g_state.prev_bike_x;
        if (v >  kSteerVelClamp) v =  kSteerVelClamp;
        if (v < -kSteerVelClamp) v = -kSteerVelClamp;
        g_state.smoothed_vel += (v - g_state.smoothed_vel) * kSteerVelSmooth;
    }
    g_state.prev_bike_x = t.bikePos.x;
    g_state.have_prev_bike_x = true;
}

// When the targeted gate changes, the previous one has just been passed:
// interpolate its lateral miss at fwdGap 0 between its last sample in front and
// this tick, and score it against the engine's real hit test.
void ScoreCrossing(const PadTick& t) {
    if (!t.bikeOk || !g_state.have_prev_ahead ||
        t.aheadSlot == g_state.prev_ahead_slot || g_state.prev_ahead_slot < 0) {
        return;
    }
    const float fwdGapNow = g_state.prev_ahead_posY - t.refY;
    const float padErrNow = g_state.prev_ahead_posX - t.bikePos.x;
    if (!(g_state.prev_ahead_fwdGap > 0.0f && fwdGapNow < g_state.prev_ahead_fwdGap)) {
        return;
    }
    float f = g_state.prev_ahead_fwdGap / (g_state.prev_ahead_fwdGap - fwdGapNow);
    if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;
    const float crossErr =
        g_state.prev_ahead_padErr + (padErrNow - g_state.prev_ahead_padErr) * f;
    const float acrx = crossErr < 0.0f ? -crossErr : crossErr;
    // The real hit radius is player.sphere_radius + pad.sphere_radius (the
    // engine's swept sphere test); the literal only when a read faulted.
    const float combinedRadius =
        (t.playerRadius > 0.0f && t.aheadRadius > 0.0f)
            ? (t.playerRadius + t.aheadRadius)
            : kAccelpadHitRadiusU;
    const bool caught = acrx <= combinedRadius;
    acclog::Write("SwoopRace",
                  "CROSS slot=%d padX=%.2f crossErr=%.2f abs=%.2f "
                  "hit=%d radius=%.1f vel=%.2f",
                  g_state.prev_ahead_slot, g_state.prev_ahead_posX,
                  crossErr, acrx, caught ? 1 : 0,
                  combinedRadius, g_state.smoothed_vel);
    // Neither game's pad script plays a sound (the boost is visual only), and
    // with the magnet catching the pads the guide tone rests most of the race:
    // one centred burst of the pad's own tone says a pad was scored.
    if (caught) acc::audio::PlayCue(kAccelpadLoopResref);
}

// The nearest pad, or after the last pad on KOTOR 2 a point straight ahead (see
// kVirtualTargetAheadU), so the mine-aware line keeps working to the finish.
void ChooseTarget(PadTick& t) {
    if (t.aheadSlot < 0 && t.bikeOk && acc::game::IsKotor2() &&
        t.bikeSpeed > kSwoopMinRaceSpeed) {
        t.virtualTarget = true;
        t.aheadPos = t.bikePos;
        t.aheadPos.y += kVirtualTargetAheadU;
    }
    t.haveTarget = (t.aheadSlot >= 0 || t.virtualTarget) && t.bikeOk;
    t.padErr   = t.haveTarget ? (t.aheadPos.x - t.bikePos.x) : 0.0f;
    t.fwdGap   = t.haveTarget ? (t.aheadPos.y - t.refY) : -1.0f;
    t.guidePos = t.aheadPos;
    t.guideErr = t.padErr;
}

// Bar-aware line (see kBarFreeClearU and the kPostContactU block), from the
// obstacle sweep's bar cache of last tick. When both detours bite the bar wins
// (a park costs 16-19 s, a mine 100 hp and a downshift).
void ApplyBarDetour(void* miniGame, PadTick& t) {
    int   bar_detour_slot = -1;
    const bool airborne = t.tunOk && t.tunnel.z > kAirborneZ;
    if (!airborne) g_state.bar_air_cleared_slot = -1;   // per-flight latch
    if (t.bikeOk && !airborne) g_state.ground_z = t.bikePos.z;
    float groundedY = t.refY;   // where the bike is next on the ground
    float barY      = 0.0f;
    const char* barWhy = "";
    if (t.haveTarget && acc::game::IsKotor2() && g_state.bar_count > 0 && t.fwdGap > 0.0f) {
        const float vWorld = t.bikeSpeed * kSpeedToWorldPerSec;   // world u/s
        if (airborne) {
            // Remaining flight from the arc: z + vz*t - g*t^2/2 = 0.
            const float vz   = SafeReadFloat(miniGame, kMiniGameJumpVelocityOffset);
            const float disc = vz * vz + 2.0f * kJumpGravity * t.tunnel.z;
            const float tLand = (vz + std::sqrt(disc > 0.0f ? disc : 0.0f)) / kJumpGravity;
            groundedY = t.refY + tLand * vWorld;
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
            if (bar.y1 + kBarLaneMarginU < t.refY) continue;           // behind the bike
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
            float f = (bar.y0 - t.refY) / t.fwdGap;
            if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;
            const float pathX   = t.bikePos.x + (t.aheadPos.x - t.bikePos.x) * f;
            const bool  crosses = pathX >= bar.x0 - kBarLaneMarginU &&
                                  pathX <= bar.x1 + kBarLaneMarginU;
            // Pad timing: a pad that lies before the bar is caught first,
            // unless it sits in the span too close in front of the bar to
            // move over afterwards.
            const bool padBehindBar = t.aheadPos.y > bar.y0;
            const bool padInSpan    = t.aheadPos.x >= bar.x0 - kBarLaneMarginU &&
                                      t.aheadPos.x <= bar.x1 + kBarLaneMarginU;
            const bool padTooClose  = !padBehindBar && padInSpan &&
                                      (bar.y0 - t.aheadPos.y) < kBarAfterPadLeadSec * vWorld;
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
                t.guidePos.x = bar.gap_x;
                t.guideErr   = bar.gap_x - t.bikePos.x;
                bar_detour_slot = bar.slot;
                barY = bar.y0;
            }
        }
    }
    t.barDetourSlot = bar_detour_slot;
    t.barY          = barY;
    if (bar_detour_slot != g_state.bar_detour_slot) {
        if (bar_detour_slot >= 0) {
            acclog::Write("SwoopRace",
                          "bar detour: slot=%d guideX=%.1f why=%s (pad slot=%d padX=%.1f padY=%.0f "
                          "bikeX=%.1f barY=%.0f airborne=%d groundY=%.0f fwdGap=%.0f)",
                          bar_detour_slot, t.guidePos.x, barWhy, t.aheadSlot, t.aheadPos.x,
                          t.aheadPos.y, t.bikePos.x, barY, airborne ? 1 : 0, groundedY, t.fwdGap);
        } else {
            acclog::Write("SwoopRace", "bar detour: clear (pad slot=%d bikeX=%.1f airborne=%d groundY=%.0f)",
                          t.aheadSlot, t.bikePos.x, airborne ? 1 : 0, groundedY);
        }
        g_state.bar_detour_slot = bar_detour_slot;
    }
}

// Mine-aware line (see kMineClearU), checked on the final line: the pad, or the
// strip the bar detour chose (a strip line once passed a mine at 4 u).
void ApplyMineDetour(PadTick& t) {
    int    detour_slot = -1;
    const float line_x   = t.guidePos.x;
    const float line_y   = (t.barDetourSlot >= 0) ? t.barY : t.aheadPos.y;
    const float lineGap  = t.haveTarget ? (line_y - t.refY) : -1.0f;
    if (t.haveTarget && acc::game::IsKotor2() && lineGap > 0.0f) {
        float  bestDy = 1e9f;
        Vector mine   = {0.0f, 0.0f, 0.0f};
        for (int m = 0; m < g_state.mine_count; ++m) {
            const Vector& mpos = g_state.mine_pos[m];
            const float dy = mpos.y - t.refY;
            if (dy < -kMineBehindU || dy > lineGap) continue;   // behind, or beyond the line's end
            const float f = dy <= 0.0f ? 0.0f : dy / lineGap;
            const float pathX = t.bikePos.x + (line_x - t.bikePos.x) * f;
            // Hysteresis: the mine already being detoured stays a detour
            // until it is behind (the detour itself moves the path off it,
            // which would flip the decision every tick).
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
            t.guidePos.x = gx;
            t.guideErr   = gx - t.bikePos.x;
        }
    }
    t.mineDetourSlot = detour_slot;
    if (detour_slot != g_state.detour_slot) {
        if (detour_slot >= 0) {
            acclog::Write("SwoopRace",
                          "detour: mine slot=%d guideX=%.1f (line x=%.1f y=%.0f pad slot=%d padX=%.1f bikeX=%.1f fwdGap=%.0f)",
                          detour_slot, t.guidePos.x, line_x, line_y, t.aheadSlot, t.aheadPos.x,
                          t.bikePos.x, t.fwdGap);
        } else {
            acclog::Write("SwoopRace", "detour: clear (pad slot=%d)", t.aheadSlot);
        }
        g_state.detour_slot = detour_slot;
    }
}

// Lateral steering magnetism: nudge the bike's lane (offset.x, +0x1c4) toward
// the guide line, gentle far, sticky near, capped per tick, on top of the
// player's own steering. offset.x maps 1:1 to world X, so the step is the
// world error, sign +1. No pull in gear 0: after a KOTOR 2 collision the bike
// waits there in front of what it hit, and the magnet dragged every relaunch
// back into the same bar; both games' scripts keep MIN_RACE_GEAR.
void ApplyMagnet(PadTick& t) {
    t.inGear = acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") >= 1;
    if (!t.haveTarget || !t.bikeOk || !t.tunOk || !t.player || !t.inGear) return;
    if (t.virtualTarget && t.mineDetourSlot < 0) return;
    if (!(t.bikeSpeed > kSwoopMinRaceSpeed)) return;
    if (!(std::fabs(t.guideErr) <= MagnetEngageU())) return;

    const acc::minigame::MagnetParams mp{
        kSwoopMagnetGainFar, kSwoopMagnetGainNear, kSwoopMagnetMaxStepU};
    float s = 1.0f - std::fabs(t.guideErr) / MagnetEngageU();
    if (s < 0.0f) s = 0.0f;
    // Hold through the crossing: in the last kSwoopMagnetHoldSec before the
    // gate the pull ramps to fully sticky, so drift toward the next gate cannot
    // peel the bike off this one before it scores. Time-based, so it is the same
    // in every gear.
    if (t.fwdGap > 0.0f && t.bikeSpeed > 1.0f) {
        const float ttc = t.fwdGap / t.bikeSpeed;                  // seconds to crossing
        if (ttc < kSwoopMagnetHoldSec) {
            const float hold = 1.0f - ttc / kSwoopMagnetHoldSec;  // 0 far -> 1 at crossing
            s += (1.0f - s) * hold;
        }
    }
    const float gain = acc::minigame::MagnetGain(s, mp);
    // Velocity brake: aim at where the bike will coast to, so closing fast
    // eases or counter-pulls before arrival and bleeds the overshoot.
    // smoothed_vel is bike-X velocity (+ right) and the error is target minus
    // bike (+ target right), so the lead damps inward motion on either side.
    const float brakedErr = t.guideErr - kSwoopMagnetBrakeTicks * g_state.smoothed_vel;
    const float newOffX = acc::minigame::MagnetStep(t.tunnel.x, -brakedErr, gain, mp);
    Vector newOff = t.tunnel;
    newOff.x = newOffX;
    acc::minigame::WriteOffsetVector(t.player, newOff);

    if (t.now - g_state.last_magnet_log_ms >= 1000) {
        g_state.last_magnet_log_ms = t.now;
        acclog::Write("SwoopAssist",
                      "magnet slot=%d padErr=%.2f vel=%.2f brakedErr=%.2f "
                      "gain=%.2f offX %.2f->%.2f bikeX=%.2f padX=%.2f fwdGap=%.1f",
                      t.aheadSlot, t.padErr, g_state.smoothed_vel, brakedErr,
                      gain, t.tunnel.x, newOffX,
                      t.bikePos.x, t.aheadPos.x, t.fwdGap);
    }
}

// The wall sound ~WallLeadTicks before the bike pins, so re-steering starts
// sooner. Only on a real overshoot: projected across the lane edge and moving
// away from the pad, so it stays quiet while racing toward a pad by the edge.
void PredictWallCue(const PadTick& t) {
    if (!t.haveTarget || !t.tunOk) return;
    const float vel       = g_state.smoothed_vel;
    const float projected = t.tunnel.x + vel * WallLeadTicks();
    const bool  toward_wall  = std::fabs(projected) >= WallEdgeOffsetU() &&
                               std::fabs(t.tunnel.x) < WallEdgeOffsetU();
    const bool  overshooting = vel * t.padErr < 0.0f;  // moving away from pad
    if (!toward_wall || !overshooting || !(std::fabs(vel) > kWallMinVel) ||
        t.now - g_state.last_wall_cue_ms < kWallCueDebounceMs) {
        return;
    }
    const int side = (vel > 0.0f) ? +1 : -1;
    Vector cue = t.listener;
    cue.x += (side > 0) ? +kWallCuePanM : -kWallCuePanM;
    acc::audio::PlayCue3DRaw(kWallCueResref, cue);
    g_state.last_wall_cue_ms = t.now;
    acclog::Write("SwoopRace",
                  "wall predict side=%s tunnelX=%.1f vel=%.2f "
                  "proj=%.1f padErr=%.1f",
                  side > 0 ? "right" : "left", t.tunnel.x, vel,
                  projected, t.padErr);
}

// KOTOR 2 wall loop (see kWallBandU): in the band, or projected across the
// edge on a real overshoot, in any gear, with hysteresis at the band edge.
void UpdateWallLoop(const PadTick& t) {
    if (!acc::game::IsKotor2() || !t.tunOk) return;
    // The projection counts only when moving away from the guide target (or
    // with none): firing on every fast swing toward a side-lane pad trained
    // the ear to ignore it.
    const float vel  = g_state.smoothed_vel;
    const float proj = t.tunnel.x + vel * WallLeadTicks();
    const bool  overshoot = !t.haveTarget || vel * t.guideErr < 0.0f;
    // The band is unconditional, except while the guide itself leads into it: a
    // half-bar's strip beside the posts (MGC03 at x -12.4, MGC06 at -13.1)
    // would otherwise ride under a permanent wall thump.
    const bool guideInBand = t.haveTarget && t.barDetourSlot >= 0 &&
                             std::fabs(t.guidePos.x) >= kWallBandU - kWallBandHystU;
    const bool projOk = overshoot && (!guideInBand || std::fabs(vel) >= kWallLoopBandMinVel);
    int side = 0;
    if ((!guideInBand && t.tunnel.x >= kWallBandU) || (projOk && proj >= kWallEdgeOffsetK2U)) {
        side = +1;
    } else if ((!guideInBand && t.tunnel.x <= -kWallBandU) ||
               (projOk && proj <= -kWallEdgeOffsetK2U)) {
        side = -1;
    }
    if (side == 0 && !guideInBand && g_state.wall_loop_side != 0 &&
        t.tunnel.x * g_state.wall_loop_side > kWallBandU - kWallBandHystU) {
        side = g_state.wall_loop_side;   // still in the hysteresis band
    }
    if (side != 0) {
        Vector cue = t.listener;
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
                          side > 0 ? "right" : "left", t.tunnel.x,
                          vel, proj, t.inGear ? 1 : 0, overshoot ? 1 : 0);
        }
    } else if (g_state.wall_loop.IsActive()) {
        g_state.wall_loop.Stop();
        acclog::Write("SwoopRace", "wall loop: stop tunnelX=%.1f", t.tunnel.x);
    }
    g_state.wall_loop_side = side;
}

// The panned steering tone (see the block above); returns whether it sounds.
// When it goes from sounding to rest, one centred "aligned" blip says the bike
// is on the line and steering can stop.
bool UpdateGuideTone(const PadTick& t) {
    // Rest radius: the catch radius for a pad; a strip target is only 5 u past
    // the bar's end, so resting that far short left the bike on the bar with
    // nothing to hear.
    const float restU = (t.barDetourSlot >= 0) ? kBarStripRestU : kSwoopGuideSilenceU;
    bool guide_active = false;
    if (t.haveTarget && t.bikeOk && std::fabs(t.guideErr) > restU) {
        const float dist = std::sqrt(DistXYZSq(t.guidePos, t.listener));
        Vector cue_pos = t.guidePos;
        cue_pos.y = t.listener.y + kSwoopGuidePanForwardRefM;
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

    if (t.haveTarget && g_state.guide_was_active && !guide_active) {
        Vector cue = t.listener;                        // centred — no pan
        cue.y = t.listener.y + kSteerAlignedFwdM;
        acc::audio::PlayCue3DRaw(kSteerAlignedResref, cue,
                              /*priorityGroup=*/0, kSteerCueVolume);
        acclog::Trace("SwoopRace", "guide aligned: padErr=%.2f slot=%d",
                      t.padErr, t.aheadSlot);
    }
    g_state.guide_was_active = guide_active;
    return guide_active;
}

void TraceTick(const PadTick& t, bool guideActive) {
    if (t.haveTarget) {
        acclog::Trace("SwoopRace",
                      "guide: padX=%.2f bikeX=%.2f padErr=%.2f vel=%.2f "
                      "guide=%d fwdGap=%.1f tunnelX=%s%.2f",
                      t.aheadPos.x, t.bikePos.x, t.padErr, g_state.smoothed_vel,
                      guideActive ? 1 : 0, t.fwdGap,
                      t.tunOk ? "" : "FAULT:", t.tunOk ? t.tunnel.x : 0.0f);
    }
    acclog::Trace("SwoopRace",
                  "accelpad scan: slots=%d accelpads=%d ahead=%d aheadSlot=%d "
                  "refY=%.1f",
                  t.slotsSeen, t.padsFound, t.padsAhead, t.aheadSlot, t.refY);
}

// This tick's gate, for next tick's crossing interpolation.
void RememberTarget(const PadTick& t) {
    if (t.haveTarget) {
        g_state.prev_ahead_slot   = t.aheadSlot;
        g_state.prev_ahead_posX   = t.aheadPos.x;
        g_state.prev_ahead_posY   = t.aheadPos.y;
        g_state.prev_ahead_padErr = t.padErr;
        g_state.prev_ahead_fwdGap = t.fwdGap;
        g_state.have_prev_ahead   = true;
    } else {
        g_state.have_prev_ahead   = false;
        g_state.prev_ahead_slot   = -1;
    }
}

}  // namespace

void TickAccelpadCues(void* miniGame) {
    void* mgoArray = ResolveMgoArray();
    if (!mgoArray) {
        acclog::Trace("SwoopRace", "accelpad cues: mgo array unresolved");
        return;
    }
    PadTick t;
    if (!ReadTickInputs(miniGame, t)) return;
    ScanEnemies(mgoArray, t);
    UpdateLateralVelocity(t);
    ScoreCrossing(t);
    ChooseTarget(t);
    ApplyBarDetour(miniGame, t);
    ApplyMineDetour(t);
    t.now = GetTickCount64();
    ApplyMagnet(t);
    PredictWallCue(t);
    UpdateWallLoop(t);
    TraceTick(t, UpdateGuideTone(t));
    RememberTarget(t);
}

}  // namespace acc::swoop_race
