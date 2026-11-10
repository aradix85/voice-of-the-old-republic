// Accelerator pad cues of the swoop race: steering onto the next pad, the
// wall warning, the assist gap and release band, per tick.

#include "minigame_swoop_audio_accelpad_internal.h"

#include "audio_bus.h"
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

// The on-pad "aligned" blip is centred (no pan), so it needs real forward depth
// to clear the near-field dead zone (cues closer than ~5 m attenuate oddly; 3 m
// was inaudible); 7 m keeps it audible and within range.
constexpr float       kSteerAlignedFwdM = 7.0f;
constexpr const char* kSteerAlignedResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopSteerAligned);
// Base volume (0..127) of the hot synthesised tone (RMS ~44-48%), set by the
// player's request; the global cue slider still scales it.
constexpr uint8_t     kSteerCueVolume = 54;

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
