// The detours of the guide line: around bars and mines (KOTOR 2) and, for the
// automatic pilot, around the buoys (KOTOR 1).

#include "minigame_swoop_audio_accelpad_internal.h"

#include "engine_area.h"      // ReadGlobalNumber: MIN_RACE_GEAR gates the magnet
#include "engine_game.h"
#include "log.h"

namespace acc::swoop_race {

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
            const float vz   = ReadFloatOr0(miniGame, kMiniGameJumpVelocityOffset);
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
            if (sticky || parked) {
                engage = true;
                barWhy = sticky ? "sticky" : "parked";
            } else if (airborne) {
                engage = crosses;
                barWhy = "landing short";
            } else if (crosses && timingOk) {
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
            acclog::Write("SwoopRace",
                          "bar detour: clear (pad slot=%d bikeX=%.1f airborne=%d groundY=%.0f)",
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
                          "detour: mine slot=%d guideX=%.1f (line x=%.1f y=%.0f pad slot=%d "
                          "padX=%.1f bikeX=%.1f fwdGap=%.0f)",
                          detour_slot, t.guidePos.x, line_x, line_y, t.aheadSlot, t.aheadPos.x,
                          t.bikePos.x, t.fwdGap);
        } else {
            acclog::Write("SwoopRace", "detour: clear (pad slot=%d)", t.aheadSlot);
        }
        g_state.detour_slot = detour_slot;
    }
}

// The automatic pilot's line around the buoys (KOTOR 1), from the obstacle
// sweep's list of last tick. The line itself is minigame_swoop_pilot.cpp's.
void ApplyRockDetour(PadTick& t) {
    int slot = -1;
    if (AutopilotOn() && t.haveTarget && t.tunOk && t.fwdPerTick > 0.0f) {
        acc::swoop_pilot::Bike bike;
        bike.x          = t.bikePos.x;
        bike.y          = t.refY;
        bike.laneX      = t.bikePos.x - t.tunnel.x;   // offset.x maps 1:1 to world X
        bike.radius     = t.playerRadius > 0.0f ? t.playerRadius : kPilotBikeRadiusU;
        bike.fwdPerTick = t.fwdPerTick > kPilotMinFwdU ? t.fwdPerTick : kPilotMinFwdU;
        const float line = acc::swoop_pilot::LineAroundRocks(
            bike, t.guidePos.x, t.aheadPos.y, g_state.rocks, g_state.rock_count,
            g_state.rock_side, slot);
        if (slot >= 0) {
            t.guidePos.x = line;
            t.guideErr   = line - t.bikePos.x;
        }
    }
    t.rockDetourSlot = slot;
    if (slot != g_state.rock_detour_slot) {
        if (slot >= 0) {
            acclog::Write("SwoopRace",
                          "pilot: buoy slot=%d passed on the %s, line x=%.1f (pad slot=%d "
                          "padX=%.1f padY=%.0f bikeX=%.1f bikeY=%.0f fwd=%.1f/tick)",
                          slot, g_state.rock_side[slot] < 0 ? "left" : "right", t.guidePos.x,
                          t.aheadSlot, t.aheadPos.x, t.aheadPos.y, t.bikePos.x, t.refY,
                          t.fwdPerTick);
        } else {
            acclog::Write("SwoopRace", "pilot: no buoy in the way (pad slot=%d bikeX=%.1f)",
                          t.aheadSlot, t.bikePos.x);
        }
        g_state.rock_detour_slot = slot;
    }
}

}  // namespace acc::swoop_race
