// When to announce a new cluster: the player's motion and the centre rule,
// which waits until the player is heading into the new region rather than
// brushing its edge.

#include "transitions_internal.h"

#include "log.h"

namespace acc::transitions {

namespace {

// Player ground velocity, sampled every kMotionSampleMs (one frame is too
// noisy, and a gap longer than kMotionStaleMs — a load, a pause — is not
// motion). Updated every tick by TrackMotion, read by the centre rule.
constexpr DWORD kMotionSampleMs = 100;
constexpr DWORD kMotionStaleMs  = 500;
// Faster than any walk or run (KOTOR 1 running: ~5.7 m/s): a jump of the
// position, e.g. the spawn after an area load (26.9 m/s logged), not motion.
constexpr float kMotionMaxMps   = 15.0f;
Vector g_motion_prev_pos   = {0.0f, 0.0f, 0.0f};
DWORD  g_motion_prev_ms    = 0;
float  g_motion_vx         = 0.0f;
float  g_motion_vy         = 0.0f;
bool   g_motion_valid      = false;

// The centre rule. A junction, a merged room or a plaza below the large-area
// size speaks exit words measured from its centroid, but the cluster is
// entered when its node becomes the nearest one, roughly halfway from the
// neighbour: up to ~9 m off-centre on wide streets, where the words are
// wrong. So the announce is held until the player is where the words are
// true:
//   - within a quarter of the cluster's footprint of its centre, or
//   - moving, with the closest point of the current path less than the lead
//     time ahead (or already behind), or
//   - no longer getting closer: past the closest point, or standing still.
// A player who leaves the cluster first hears nothing, as with the dwell
// gate. Large areas keep the delayed place line (their centre is rarely
// reached); corridors and dead ends keep the dwell (an axis word is true
// anywhere in a run, and a dead end is worth hearing early).
//
// "Near" uses the cluster's floor footprint, not its node spread, so a
// single-node crossing has a real radius (a hall ~11 m wide: 2.8 m).
//
// The lead: what has to happen at the centre is the player STOPPING, so the
// exit words are heard from there. Firing only after a stall left a running
// player 4-5 m past the centre (300 ms to prove the stall, the noun, then
// the reaction to let go of the key, at ~4.5 m/s). So a moving player is
// announced when the closest point of the path lies less than the noun's
// length plus a reaction allowance ahead: walking slowly fires close to the
// centre, running further out. The stall numbers only define "no longer
// getting closer": a step must gain more than walk jitter, and the stall
// must outlast a frame hitch. kCentreReactionMs is the one human constant:
// the time from hearing the noun to the key coming up.
constexpr float kCentreRadiusFraction = 0.25f;
constexpr float kCentreProgressEpsM   = 0.05f;
constexpr DWORD kCentreNoCloserMs     = 300;
constexpr DWORD kCentreReactionMs     = 500;
// Time to hear the region noun at the default rate.
constexpr DWORD kCentreLeadMs         = 420;
// Below this the player counts as standing (the stall test's case).
constexpr float kCentreMovingMps      = 0.5f;
Vector g_pending_centre_pos     = {0.0f, 0.0f, 0.0f};
float  g_pending_centre_radius  = 0.0f;
float  g_pending_centre_best    = 0.0f;
DWORD  g_pending_centre_best_ms = 0;

float CentreDistance(const Vector& pos) {
    return DistXY(pos, g_pending_centre_pos);
}

}  // namespace

void TrackMotion(const Vector& pos) {
    const DWORD now = GetTickCount();
    if (g_motion_prev_ms == 0) {
        g_motion_prev_pos = pos;
        g_motion_prev_ms  = now;
        return;
    }
    const DWORD dt = now - g_motion_prev_ms;
    if (dt < kMotionSampleMs) return;
    if (dt <= kMotionStaleMs) {
        g_motion_vx    = (pos.x - g_motion_prev_pos.x) * 1000.0f / dt;
        g_motion_vy    = (pos.y - g_motion_prev_pos.y) * 1000.0f / dt;
        g_motion_valid = (g_motion_vx * g_motion_vx +
                          g_motion_vy * g_motion_vy) <=
                         kMotionMaxMps * kMotionMaxMps;
    } else {
        g_motion_valid = false;
    }
    g_motion_prev_pos = pos;
    g_motion_prev_ms  = now;
}

void ResetMotion() {
    g_motion_prev_ms = 0;
    g_motion_valid   = false;
}

// Start the centre rule for a cluster that has just become pending.
void ArmCentreRule(int clusterId, const acc::room_topology::ClusterInfo& info,
                   const Vector& pos, DWORD sinceMs) {
    g_pending_centre_pos     = info.centre;
    g_pending_centre_radius  = info.footprintM * kCentreRadiusFraction;
    g_pending_centre_best    = CentreDistance(pos);
    g_pending_centre_best_ms = sinceMs;
    acclog::Write("Transition",
                  "cluster -> %d centre rule (kind=%d extent=%.1fm "
                  "footprint=%.1fm radius=%.1fm dist=%.1fm)",
                  clusterId, info.kind, info.extentM, info.footprintM,
                  g_pending_centre_radius, g_pending_centre_best);
}

// True once the pending centre-rule cluster may speak. Tracks the closest
// distance reached so far; called every tick while the cluster is pending.
bool CentreRuleReady(int clusterId, const Vector& pos) {
    const DWORD now  = GetTickCount();
    const float dist = CentreDistance(pos);
    if (dist <= g_pending_centre_radius) {
        acclog::Write("Transition",
                      "cluster -> %d centre rule met: near (dist=%.1fm <= "
                      "%.1fm)", clusterId, dist, g_pending_centre_radius);
        return true;
    }
    if (g_motion_valid) {
        const float speedSq = g_motion_vx * g_motion_vx +
                              g_motion_vy * g_motion_vy;
        if (speedSq >= kCentreMovingMps * kCentreMovingMps) {
            const float leadS = (kCentreLeadMs + kCentreReactionMs) / 1000.0f;
            // Time until the closest point of the current path; negative
            // once it is behind.
            const float rx = g_pending_centre_pos.x - pos.x;
            const float ry = g_pending_centre_pos.y - pos.y;
            const float tClosest = (rx * g_motion_vx + ry * g_motion_vy) /
                                   speedSq;
            if (tClosest <= leadS) {
                const float cx = rx - g_motion_vx * tClosest;
                const float cy = ry - g_motion_vy * tClosest;
                acclog::Write("Transition",
                              "cluster -> %d centre rule met: lead (dist=%.1fm "
                              "speed=%.1fm/s closest in %.2fs at %.1fm, "
                              "lead=%.2fs noun=%lums radius=%.1fm)",
                              clusterId, dist, std::sqrt(speedSq), tClosest,
                              std::sqrt(cx * cx + cy * cy), leadS,
                              (unsigned long)kCentreLeadMs,
                              g_pending_centre_radius);
                return true;
            }
        }
    }
    if (dist < g_pending_centre_best - kCentreProgressEpsM) {
        g_pending_centre_best    = dist;
        g_pending_centre_best_ms = now;
        return false;
    }
    if ((now - g_pending_centre_best_ms) >= kCentreNoCloserMs) {
        acclog::Write("Transition",
                      "cluster -> %d centre rule met: no closer for %lums "
                      "(dist=%.1fm best=%.1fm radius=%.1fm)",
                      clusterId, (unsigned long)(now - g_pending_centre_best_ms),
                      dist, g_pending_centre_best, g_pending_centre_radius);
        return true;
    }
    return false;
}

}  // namespace acc::transitions
