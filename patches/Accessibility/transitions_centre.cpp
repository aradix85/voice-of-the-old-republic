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

float CentreDistance(const Vector& pos) {
    return DistXY(pos, g_pending_centre_pos);
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
            const DWORD nounMs = kCentreLeadMs;
            const float leadS = (nounMs + kCentreReactionMs) / 1000.0f;
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
                              (unsigned long)nounMs, g_pending_centre_radius);
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
