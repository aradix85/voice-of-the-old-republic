// The steering magnet and the wall cues of the swoop race.

#include "minigame_swoop_audio_accelpad_internal.h"

#include "audio_bus.h"
#include "engine_area.h"      // ReadGlobalNumber: MIN_RACE_GEAR gates the magnet
#include "engine_game.h"
#include "log.h"

namespace acc::swoop_race {

namespace {

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

}  // namespace acc::swoop_race
