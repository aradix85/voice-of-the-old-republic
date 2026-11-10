// K2 bars and mines of the swoop race: the free gap beside a half bar, the
// jump-now cue (and its parked variant), and the mine-pass log.

#include "minigame_swoop_audio_internal.h"

#include "audio_bus.h"
#include "log.h"

namespace acc::swoop_race {

namespace {

// ============================================================================
// KOTOR 2 half-bar free-side strips (kPostContactU block) + the mine-pass log.
// ============================================================================

// Lateral reach of mine `m` at the bike's ground height: sqrt(R² − dz²) plus
// slack, R = the measured combined reach (kMineCollisionRadiusU — the live
// sphere_radius is NOT the collision radius, see there).
float MineLateralReach(int m, float bikeZ, float /*playerRadius*/) {
    const float R  = kMineCollisionRadiusU;
    const float dz = g_state.mine_pos[m].z - bikeZ;
    const float sq = R * R - dz * dz;
    return (sq > 0.0f ? std::sqrt(sq) : 0.0f) + kBarStripSlackU;
}

// One side's strip [lo, hi] minus every mine near the bar. Returns the width
// of the widest surviving piece (0 = none) and its bounds.
float WidestStripPiece(float lo, float hi, float barY, float bikeZ, float playerRadius,
                       float& outLo, float& outHi) {
    float pl[8], ph[8]; int n = 0;
    if (lo < hi) { pl[0] = lo; ph[0] = hi; n = 1; }
    for (int m = 0; m < g_state.mine_count && n > 0; ++m) {
        if (std::fabs(g_state.mine_pos[m].y - barY) > kBarMineWindowU) continue;
        const float reach = MineLateralReach(m, bikeZ, playerRadius);
        const float mlo = g_state.mine_pos[m].x - reach;
        const float mhi = g_state.mine_pos[m].x + reach;
        float nl[8], nh[8]; int k = 0;
        for (int i = 0; i < n; ++i) {
            if (mhi <= pl[i] || mlo >= ph[i]) {            // untouched piece
                if (k < 8) { nl[k] = pl[i]; nh[k] = ph[i]; ++k; }
                continue;
            }
            if (mlo > pl[i] && k < 8) { nl[k] = pl[i]; nh[k] = mlo;   ++k; }
            if (mhi < ph[i] && k < 8) { nl[k] = mhi;   nh[k] = ph[i]; ++k; }
        }
        for (int i = 0; i < k; ++i) { pl[i] = nl[i]; ph[i] = nh[i]; }
        n = k;
    }
    float best = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float w = ph[i] - pl[i];
        if (w > best) { best = w; outLo = pl[i]; outHi = ph[i]; }
    }
    return best;
}

// Fills bar.has_gap / gap_x / gap_side (-1 left strip, +1 right strip).
void ComputeBarGap(SpatialAudioState::BarInfo& bar, float bikeZ, float playerRadius) {
    float lLo = 0.0f, lHi = 0.0f, rLo = 0.0f, rHi = 0.0f;
    const float lw = WidestStripPiece(-kPostContactU + kBarStripSlackU, bar.x0 - kBarFreeClearU,
                                      bar.y0, bikeZ, playerRadius, lLo, lHi);
    const float rw = WidestStripPiece(bar.x1 + kBarFreeClearU, kPostContactU - kBarStripSlackU,
                                      bar.y0, bikeZ, playerRadius, rLo, rHi);
    bar.has_gap = false; bar.gap_x = 0.0f; bar.gap_side = 0;
    const bool lOk = lw >= kBarStripMinU, rOk = rw >= kBarStripMinU;
    if (!lOk && !rOk) return;
    const bool useLeft = lOk && (!rOk || lw >= rw);
    bar.has_gap  = true;
    bar.gap_side = useLeft ? -1 : +1;
    if (useLeft) bar.gap_x = (lw >= kBarStripWideU) ? lHi : 0.5f * (lLo + lHi);
    else         bar.gap_x = (rw >= kBarStripWideU) ? rLo : 0.5f * (rLo + rHi);
}

}  // namespace

// Mine-pass log: per mine, the closest 3-D approach while the mine is within
// kMinePassWindowU ahead or behind, logged once when it falls behind, with
// the hp before and after. Hits at a 2-D distance inside R but a 3-D one
// outside it would say the collision is flat; clean passes the other way
// round say it is spherical (the model kPostContactU's block assumes).
void TrackMinePasses(void* player, const Vector& bikePos, float playerRadius) {
    const int hp = static_cast<int>(SafeReadU32(player, kFollowerHpOffset));
    for (int m = 0; m < g_state.mine_count; ++m) {
        const int slot = g_state.mine_slot[m];
        if (slot < 0 || slot >= kMgoArraySlotCount) continue;
        const Vector& mp = g_state.mine_pos[m];
        const float dx = mp.x - bikePos.x, dy = mp.y - bikePos.y, dz = mp.z - bikePos.z;
        if (dy > kMinePassWindowU) continue;                     // still far ahead
        if (dy < -kMinePassWindowU) {
            if (!g_state.mine_pass_open[slot]) continue;
            g_state.mine_pass_open[slot] = false;
            const float pdx = g_state.mine_pass_dx[slot], pdy = g_state.mine_pass_dy[slot],
                        pdz = g_state.mine_pass_dz[slot];
            const int hp0 = g_state.mine_pass_hp0[slot];
            acclog::Write("SwoopRace",
                          "mine pass: slot=%d mine=(%.1f,%.1f,%.1f) r=%.2f bikeR=%.2f bikeZ=%.2f "
                          "minD3=%.2f at dx=%.2f dy=%.2f dz=%.2f d2=%.2f hp %d->%d hit=%d",
                          slot, mp.x, mp.y, mp.z, g_state.mine_radius[m], playerRadius,
                          bikePos.z, g_state.mine_pass_min_d3[slot], pdx, pdy, pdz,
                          std::sqrt(pdx * pdx + pdy * pdy), hp0, hp,
                          (hp0 > 0 && hp < hp0) ? 1 : 0);
            continue;
        }
        const float d3 = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!g_state.mine_pass_open[slot]) {
            g_state.mine_pass_open[slot]   = true;
            g_state.mine_pass_min_d3[slot] = d3;
            g_state.mine_pass_dx[slot] = dx; g_state.mine_pass_dy[slot] = dy; g_state.mine_pass_dz[slot] = dz;
            g_state.mine_pass_hp0[slot]    = hp;
        } else if (d3 < g_state.mine_pass_min_d3[slot]) {
            g_state.mine_pass_min_d3[slot] = d3;
            g_state.mine_pass_dx[slot] = dx; g_state.mine_pass_dy[slot] = dy; g_state.mine_pass_dz[slot] = dz;
        }
    }
}

// Parked right in front of this bar, inside its span: classify the escape
// (see ParkedBar in the header) for the stuck hint, and, off the wall with
// no strip, play the centred beep once per park: "jump first, then W". At
// the wall the beep stays silent (a jump there hits the pillar; rounds 15/16
// lost 7 and 11 relaunches that way) and the wall loop is the cue.
void HandleParkedBar(int i, const ObstacleView& v, HazardInfo& hz,
                     const SpatialAudioState::BarInfo& cur, float gap, float bikeX) {
    if (gap <= -kBarLaneMarginU || gap > kParkedBarAheadU ||
        g_state.parked_bar != ParkedBar::None) {
        return;
    }
    if (bikeX >= kParkedWallU) {
        g_state.parked_bar = ParkedBar::WallRight;
    } else if (bikeX <= -kParkedWallU) {
        g_state.parked_bar = ParkedBar::WallLeft;
    } else if (cur.has_gap) {
        g_state.parked_bar = ParkedBar::Strip;
    } else {
        g_state.parked_bar = ParkedBar::MustJump;
        if (!hz.park_cued) {
            hz.park_cued = 1;
            acc::audio::PlayCue(kJumpNowCueResref);
            acclog::Write("SwoopRace",
                          "parked beep: slot=%d name=[%s] bar y=%.1f gap=%.1f bikeX=%.2f",
                          i, v.name ? v.name : "(null)", v.y0, gap, bikeX);
        }
    }
}

// The jump lead: the single-bar constant, or, when the next bar behind this
// one is close enough that one jump clears both at this speed with a press
// window of at least kPairMinWindowSec, the later "pair" lead that centres
// the usual reaction in that window (see kJumpReactionSec). The bar list is
// last tick's.
float JumpLead(int slot, float y0, float vWorld, bool& pair) {
    pair = false;
    float nextY0 = 0.0f, nextY1 = 0.0f;
    bool haveNext = false;
    for (int b = 0; b < g_state.bar_count; ++b) {
        const SpatialAudioState::BarInfo& nb = g_state.bars[b];
        if (nb.slot == slot || nb.y0 <= y0) continue;
        if (!haveNext || nb.y0 < nextY0) {
            nextY0 = nb.y0;
            nextY1 = nb.y1;
            haveNext = true;
        }
    }
    if (!haveNext) return kJumpCueLeadSec;
    const float reach  = (nextY1 + kBarLaneMarginU - y0) / vWorld;
    const float window = kJumpOverBarUntilSec - reach - kJumpOverBarFromSec;
    if (window < kPairMinWindowSec) return kJumpCueLeadSec;
    pair = true;
    return kJumpOverBarFromSec + 0.5f * window + kJumpReactionSec;
}

// First beep: "press now". Second beep: the bike was airborne from an
// earlier press and has just landed with the bar still ahead inside the
// window: press again.
void MaybeJumpCue(int i, const ObstacleView& v, HazardInfo& hz,
                  const SpatialAudioState::BarInfo& cur, float gap, float bikeX,
                  const ObstacleTick& t) {
    const float vWorld = t.bikeSpeed * kSpeedToWorldPerSec;  // world u/s
    const float ttc    = gap / vWorld;                       // real seconds
    bool pair = false;
    const float lead = JumpLead(i, v.y0, vWorld, pair);
    const bool first  = hz.jump_cued == 0 && ttc <= lead;
    const bool repeat = hz.jump_cued == 1 && t.justLanded &&
                        ttc <= lead && ttc >= kJumpCueMinLeadSec;
    if (!first && !repeat) return;
    ++hz.jump_cued;
    // Free side = the surviving strip (posts and mines removed). With a strip
    // the beep sits at the strip's real lateral offset, placed like the guide
    // tone (world x, kSwoopGuidePanForwardRefM ahead), so its angle reads as
    // distance in the vocabulary of the pad lanes: narrow is "four units,
    // lean", wide "twelve, jump unless already moving". No strip: centred,
    // "you must jump".
    const int side = cur.has_gap ? cur.gap_side : 0;
    if (side == 0) {
        acc::audio::PlayCue(kJumpNowCueResref);
    } else {
        Vector cue = t.listener;
        cue.x  = cur.gap_x;
        cue.y += kSwoopGuidePanForwardRefM;
        acc::audio::PlayCue3DRaw(kJumpNowCueResref, cue);
    }
    acclog::Write("SwoopRace",
                  "jump cue%s: slot=%d name=[%s] bar y=%.1f x[%.1f..%.1f] "
                  "gap=%.1f speed=%.1f ttc=%.2fs lead=%.2fs pair=%d bikeX=%.2f side=%s "
                  "stripX=%.1f offset=%.1f",
                  repeat ? " (again)" : "", i, v.name ? v.name : "(null)",
                  v.y0, v.x0, v.x1, gap, t.bikeSpeed, ttc, lead, pair ? 1 : 0, bikeX,
                  side == 0 ? "centre" : side < 0 ? "left" : "right",
                  side == 0 ? 0.0f : cur.gap_x,
                  side == 0 ? 0.0f : cur.gap_x - bikeX);
}

// A low bar (K2): the jump-now cue, never the warn loop.
void HandleBar(int i, const ObstacleView& v, HazardInfo& hz, ObstacleTick& t) {
    ++t.barsSeen;
    SpatialAudioState::BarInfo cur;
    cur.slot = i; cur.x0 = v.x0; cur.x1 = v.x1; cur.y0 = v.y0; cur.y1 = v.y1;
    ComputeBarGap(cur, g_state.ground_z, t.playerRadius);
    if (t.newBarCount < kMgoArraySlotCount) t.newBars[t.newBarCount++] = cur;
    if (!g_state.bar_gap_diag_emitted && g_state.mine_count > 0) {
        acclog::Write("SwoopRace",
                      "bar strip: slot=%d name=[%s] y=%.0f x[%.1f..%.1f] gap=%d side=%s "
                      "target=%.1f (bikeZ=%.1f bikeR=%.2f)",
                      i, v.name ? v.name : "(null)", v.y0, v.x0, v.x1, cur.has_gap ? 1 : 0,
                      cur.gap_side < 0 ? "left" : cur.gap_side > 0 ? "right" : "none",
                      cur.gap_x, g_state.ground_z, t.playerRadius);
    }
    const float gap = v.y0 - (t.bikeOk ? t.bikePos.y : t.listener.y);
    if (gap < -kObstacleForwardMargin || gap > kObstacleCueRangeM) {
        hz.jump_cued = 0;   // passed or far: re-arm
        hz.park_cued = 0;
        return;
    }
    const float bikeX = t.bikeOk ? t.bikePos.x : t.listener.x;
    const bool inSpan = bikeX >= v.x0 - kBarLaneMarginU && bikeX <= v.x1 + kBarLaneMarginU;
    if (t.parked) {
        hz.jump_cued = 0;   // re-arm for the relaunch
        if (inSpan) HandleParkedBar(i, v, hz, cur, gap, bikeX);
        return;
    }
    hz.park_cued = 0;
    if (!inSpan || t.bikeSpeed <= 1.0f || gap <= 0.0f) return;
    MaybeJumpCue(i, v, hz, cur, gap, bikeX, t);
}

}  // namespace acc::swoop_race
