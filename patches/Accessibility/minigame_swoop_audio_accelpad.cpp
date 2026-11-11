// The accelerator-pad cues of the swoop race, per tick: the panned guide tone
// toward the next pad (or a detour point), the "aligned" blip on reaching it,
// and the crossing score. The detours and the magnet and wall cues live in
// minigame_swoop_audio_accelpad_detour.cpp and _assist.cpp.
//
// The guide is our own acc_boost tone, not the bike's hum: the swoop Player's
// <Engine> resref is empty in every area, so player+0x144 holds no source we
// could pan. It aims straight at the next pad's lane, since the pads are
// scattered gates rather than a racing line. The pan keeps the target's true X
// and Z but a fixed forward depth (kSwoopGuidePanForwardRefM), so it encodes
// the lane offset alone; loudness rises as the target nears. The tone rests
// within about the catch radius (kSwoopGuideSilenceU), where the magnet
// finishes: the player positions coarsely by ear and never has to null the pan.

#include "minigame_swoop_audio_accelpad_internal.h"

#include "audio_bus.h"
#include "engine_game.h"
#include "engine_player.h"  // the listener is the camera: the engine swaps the
                            // player creature during the race
#include "log.h"

namespace acc::swoop_race {

namespace {

// acc_boost (Override\acc_boost.wav): the first 0.3 s of mgs_basethrust03,
// without the falling tail that sounds like the bike powering down. Under its
// own name so it cannot collide with a vanilla use of the original.
constexpr const char* kAccelpadLoopResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopAccelpadBoost);

// The obstacles' 300 m horizon: first heard ~4.3 s ahead in gear 1, ~1.6 s in
// gear 3. kForwardCompression maps this range onto the band floor
// (kSwoopCueMinVolDistM); change both together.
constexpr float kAccelpadCueRangeM = 300.0f;

// Light smoothing of the noisy per-tick velocity (heavier lags ~3 ticks), and
// a clamp that rejects one-tick bike-X glitches.
constexpr float kSteerVelSmooth = 0.50f;
constexpr float kSteerVelClamp  = 8.0f;
// The lane is about +-20 wide: a pad more than 60 u to the side is a junk read.
constexpr float kMaxPadLateral  = 60.0f;

// The centred "aligned" blip needs forward depth to clear the near-field dead
// zone (3 m was inaudible).
constexpr float       kSteerAlignedFwdM = 7.0f;
constexpr const char* kSteerAlignedResref =
    acc::audio::GetNavCueResref(acc::audio::NavCue::SwoopSteerAligned);
// Base volume (0..127) of the hot synthesised tone; the cue slider scales it.
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
    t.player = ReadPtrOrNull(miniGame, kMiniGamePlayerOffset);
    t.bikeOk = ReadFollowerPosition(t.player, t.bikePos);
    t.tunOk  = TryReadVector(t.player, kMiniPlayerOffsetVectorOffset, &t.tunnel);
    t.refY   = t.bikeOk ? t.bikePos.y : t.listener.y;
    // Forward speed gates the magnet at the start hold; sphere_radius is the
    // player's half of the real hit radius.
    t.bikeSpeed    = ReadFloatOr0(t.player, kFollowerSpeedOffset);
    t.playerRadius = ReadFloatOr0(t.player, kFollowerSphereRadiusOffset);
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
        void* obj = ReadPtrOrNull(mgoArray,
                                kMgoArrayObjectsOffset +
                                static_cast<size_t>(i) * sizeof(void*));
        if (!obj) continue;
        ++t.slotsSeen;

        void* enemy = CallDowncast(obj, kVtableSlotAsEnemy);
        if (!enemy) continue;

        Vector pos;
        if (!ReadFollowerPosition(enemy, pos)) continue;

        const char* modelName = ReadAurObjectName(ReadFollowerModelGob(enemy));
        const bool isMine = IsMineModel(modelName);

        // First fire: every enemy once in the log, for the track inventory.
        if (!g_state.accelpad_diag_emitted) {
            void* vt = ReadPtrOrNull(enemy, 0);
            acclog::Write("SwoopRace",
                          "%s slot=%d ptr=%p vtable=%p model=[%s] "
                          "pos=(%.2f,%.2f,%.2f) radius=%.2f",
                          isMine ? "mine" : "accelpad", i, enemy, vt,
                          modelName ? modelName : "(null)", pos.x, pos.y, pos.z,
                          ReadFloatOr0(enemy, kFollowerSphereRadiusOffset));
        }

        if (isMine) {
            ++t.minesFound;
            if (g_state.mine_count < kMgoArraySlotCount) {
                g_state.mine_slot[g_state.mine_count]   = i;
                g_state.mine_pos[g_state.mine_count]    = pos;
                g_state.mine_radius[g_state.mine_count] =
                    ReadFloatOr0(enemy, kFollowerSphereRadiusOffset);
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
            t.aheadRadius = ReadFloatOr0(enemy, kFollowerSphereRadiusOffset);
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

// The panned steering tone (see the top of the file); returns whether it
// sounds.
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
