// The steering magnet and the wall cues of the swoop race.

#include "minigame_swoop_audio_accelpad_internal.h"

#include "audio_bus.h"
#include "engine_area.h"  // ReadGlobalNumber: MIN_RACE_GEAR
#include "engine_game.h"
#include "log.h"
#include "minigame_magnet.h"

namespace acc::swoop_race {

namespace {

// The magnet engages only within this lateral error of the guide line: the one
// difficulty knob. The aim is the sighted player's challenge: the player closes
// the lane by ear down to this gap, and only then does the magnet finish it.
// Smaller asks more precision by ear, larger lets the magnet cover more. On
// Taris 16 u pulls in the near misses at 9-14 u while the 16-23 u swings still
// need the player. KOTOR 2 lays its pads in three lanes 11 u apart, so 16 u put
// every next pad inside the gap and the magnet made every lane change (and
// walked the bike into mines and half-bars); at 6 u a one-lane hop first needs
// a 5 u swing by ear. The hit itself stays the engine's sphere test.
constexpr float kSwoopAssistGapU   = 16.0f;
constexpr float kSwoopAssistGapK2U = 6.0f;

// The pull, sized for the vanilla 5 u hit radius: gentle at the engage edge,
// sticky on the line.
constexpr float kSwoopMagnetGainFar  = 0.15f;
constexpr float kSwoopMagnetGainNear = 0.50f;
// Per-tick cap, just above the player's own peak bank (~4.7 u/tick at the
// ~18 Hz tick): enough to arrest an overshoot against a held steer key, while
// the player's bank (+-10/tick) still overrides it everywhere else.
constexpr float kSwoopMagnetMaxStepU = 5.0f;
// Velocity brake lead, in ticks: enough to anticipate an overshoot, not so
// much that the bike stops short.
constexpr float kSwoopMagnetBrakeTicks = 1.5f;
// The last stretch before a gate (~6 ticks) in which the pull goes fully
// sticky; seconds, so it is the same in every gear.
constexpr float kSwoopMagnetHoldSec = 0.35f;

float MagnetEngageU() {
    return acc::game::IsKotor2() ? kSwoopAssistGapK2U : kSwoopAssistGapU;
}

float WallLeadTicks() {
    return acc::game::IsKotor2() ? kWallLeadTicksK2 : kWallLeadTicks;
}

float WallEdgeOffsetU() {
    return acc::game::IsKotor2() ? kWallEdgeOffsetK2U : kWallEdgeOffsetU;
}

// How hard the magnet pulls this tick, and on which error.
struct Pull {
    float gain;
    float err;
};

// The pull at closeness `s` (0 at the engage edge, 1 on the line).
Pull MagnetPull(const PadTick& t, float s, const acc::minigame::MagnetParams& mp) {
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
    // Velocity brake: aim at where the bike will coast to, so closing fast
    // eases or counter-pulls before arrival and bleeds the overshoot.
    // smoothed_vel is bike-X velocity (+ right) and the error is target minus
    // bike (+ target right), so the lead damps inward motion on either side.
    return {acc::minigame::MagnetGain(s, mp),
            t.guideErr - kSwoopMagnetBrakeTicks * g_state.smoothed_vel};
}

}  // namespace

// Lateral steering magnetism. Cues that only inform plateaued at a third of
// the gates, because a full-lane swing in under a second is too little time;
// the magnet writes the bike's lane (offset.x, +0x1c4) toward the guide line,
// gentle far, sticky near, capped per tick, on top of the player's own
// steering. offset.x maps 1:1 to world X, so the step is the
// world error, sign +1. No pull in gear 0: after a KOTOR 2 collision the bike
// waits there in front of what it hit, and the magnet dragged every relaunch
// back into the same bar; both games' scripts keep MIN_RACE_GEAR.
//
// With the automatic pilot the magnet has no engage gap and is fully sticky
// from any distance: it steers the whole race.
void ApplyMagnet(PadTick& t) {
    t.inGear = acc::engine::ReadGlobalNumber("MIN_RACE_GEAR") >= 1;
    if (!t.haveTarget || !t.bikeOk || !t.tunOk || !t.player || !t.inGear) return;
    if (t.virtualTarget && t.mineDetourSlot < 0) return;
    if (!(t.bikeSpeed > kSwoopMinRaceSpeed)) return;
    const bool pilot = AutopilotOn();
    if (!pilot && !(std::fabs(t.guideErr) <= MagnetEngageU())) return;

    const acc::minigame::MagnetParams mp{
        kSwoopMagnetGainFar, kSwoopMagnetGainNear, kSwoopMagnetMaxStepU};
    float s = pilot ? 1.0f : 1.0f - std::fabs(t.guideErr) / MagnetEngageU();
    if (s < 0.0f) s = 0.0f;
    const Pull pull = MagnetPull(t, s, mp);
    const float newOffX = acc::minigame::MagnetStep(t.tunnel.x, -pull.err, pull.gain, mp);
    Vector newOff = t.tunnel;
    newOff.x = newOffX;
    acc::minigame::WriteOffsetVector(t.player, newOff);

    if (t.now - g_state.last_magnet_log_ms >= 1000) {
        g_state.last_magnet_log_ms = t.now;
        acclog::Write("SwoopAssist",
                      "magnet%s slot=%d padErr=%.2f vel=%.2f brakedErr=%.2f "
                      "gain=%.2f offX %.2f->%.2f bikeX=%.2f padX=%.2f fwdGap=%.1f",
                      pilot ? " (pilot)" : "", t.aheadSlot, t.padErr,
                      g_state.smoothed_vel, pull.err,
                      pull.gain, t.tunnel.x, newOffX,
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

}  // namespace acc::swoop_race
