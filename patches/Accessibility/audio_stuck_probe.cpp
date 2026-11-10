// The stuck-direction probe: when the leader walks without getting anywhere,
// or circles, speak once which of the eight directions are free.

#include "audio_stuck_probe.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "engine_area.h"
#include "log.h"
#include "spatial_wall_surfaces.h"
#include "speech.h"
#include "strings.h"

namespace acc::audio::footstep_suppress {

namespace {

// Stuck-direction probe gating: leader-footstep freshness ⇒ walking;
// 0.5 m over 5 s window ⇒ no progress despite walk-anim. Probe latches
// once per episode; re-arms on meaningful displacement. (Window raised
// 2 s → 5 s 2026-07-16: the free-directions announce fired too eagerly
// during normal wall-hugging navigation — user wants it only after a
// genuinely sustained stall.)
constexpr uint64_t kStuckWindowMs                 = 5000;
constexpr float    kAnnounceDisplacementMeters    = 0.5f;
constexpr float    kAnnounceDisplacementMetersSq  =
    kAnnounceDisplacementMeters * kAnnounceDisplacementMeters;

// 1.5 m probe ≈ 2 character lengths; 0.5 m body clearance covers
// creature capsules (~0.4 m radius) plus margin.
constexpr float    kProbeDistanceMeters           = 1.5f;
constexpr float    kBodyClearanceMeters           = 0.5f;
constexpr float    kBodyClearanceMetersSq         =
    kBodyClearanceMeters * kBodyClearanceMeters;
constexpr float    kSelfPositionEpsilonSq         = 0.04f;   // 0.2 m
constexpr int      kMaxNearbyBodies               = 32;

Vector   g_progress_checkpoint_pos = {0.0f, 0.0f, 0.0f};
uint64_t g_progress_checkpoint_ms  = 0;
bool     g_stuck_announced         = false;

// ---------- Circling-detection state ------------------------------------
//
// Existing TickStuckAnnounce gates on "displaced < 0.5m over kStuckWindowMs" — fires
// when the player is wedged motionless. In the 2026-05-27 cantina
// reproduction the player is moving ~3m per rotation pivot, easily
// exceeding the 0.5m threshold, so the checkpoint resets on every
// window expiration and the stuck-announce never fires — even though
// the user is unmistakably going in circles inside a 6×9m area.
//
// Circling detector measures path length (sum of per-tick |delta|)
// vs net displacement (|end - start|). High ratio = lots of motion,
// little net progress = oscillating in obstacles.
// Window raised 4 s → 5 s (and min-path scaled with it) 2026-07-16,
// same complaint as kStuckWindowMs: the circling path was the actual
// source of nearly all "free" fires in the 2026-07-16 session logs and
// interrupted deliberate tight manoeuvring. Longer window = the player
// must keep going nowhere for a full 5 s before the probe speaks.
constexpr uint64_t kCirclingWindowMs       = 5000;   // 5s window
constexpr float    kCirclingMinPathMeters  = 5.0f;   // need real motion first
                                                     // (5m at 1m/s ⇒ walking)
constexpr float    kCirclingMaxNetMeters   = 2.5f;   // and went nowhere
constexpr float    kCirclingPathRatio      = 2.5f;   // path/net >= ratio

Vector   g_circling_anchor_pos = {0.0f, 0.0f, 0.0f};
uint64_t g_circling_anchor_ms  = 0;
Vector   g_circling_prev_pos   = {0.0f, 0.0f, 0.0f};
float    g_circling_path_len   = 0.0f;
bool     g_circling_announced  = false;

// 8 cardinals, +X = East, +Y = North. Order: N, NE, E, SE, S, SW, W, NW.
constexpr float kSqrt2Over2 = 0.70710678f;
struct ProbeDir { float x, y; acc::strings::Id strId; };
const ProbeDir kProbeDirs[8] = {
    { 0.0f,         1.0f,         acc::strings::Id::DirNorth },
    { kSqrt2Over2,  kSqrt2Over2,  acc::strings::Id::DirNortheast },
    { 1.0f,         0.0f,         acc::strings::Id::DirEast },
    { kSqrt2Over2, -kSqrt2Over2,  acc::strings::Id::DirSoutheast },
    { 0.0f,        -1.0f,         acc::strings::Id::DirSouth },
    {-kSqrt2Over2, -kSqrt2Over2,  acc::strings::Id::DirSouthwest },
    {-1.0f,         0.0f,         acc::strings::Id::DirWest },
    {-kSqrt2Over2,  kSqrt2Over2,  acc::strings::Id::DirNorthwest },
};

// Nearby blocker bodies (creatures + placeables) within the probe's reach.
// Self is excluded by position epsilon instead of handle compare. Returns
// the count written to bodies.
int CollectNearbyBodies(const Vector& pos, Vector* bodies, int cap) {
    int bodyCount = 0;
    const float bodyMaxReach = kProbeDistanceMeters + kBodyClearanceMeters;
    const float bodyMaxReachSq = bodyMaxReach * bodyMaxReach;
    if (void* area = acc::engine::GetCurrentArea()) {
        acc::engine::AreaObjectIterator it(area);
        for (void* obj = it.Next(); obj; obj = it.Next()) {
            if (bodyCount >= cap) break;
            const int kind = acc::engine::GetObjectKind(obj);
            if (kind != static_cast<int>(acc::engine::GameObjectKind::Creature) &&
                kind != static_cast<int>(acc::engine::GameObjectKind::Placeable)) {
                continue;
            }
            Vector p;
            if (!acc::engine::GetObjectPosition(obj, p)) continue;
            const float ddx = p.x - pos.x;
            const float ddy = p.y - pos.y;
            const float dsq = ddx * ddx + ddy * ddy;
            if (dsq < kSelfPositionEpsilonSq) continue;  // skip self
            if (dsq > bodyMaxReachSq) continue;          // out of probe reach
            bodies[bodyCount++] = p;
        }
    }
    return bodyCount;
}

// What blocks the probe ray pos -> b: "wall", "body", or nullptr when free.
const char* ProbeBlocker(const Vector& pos, const Vector& b,
                         const acc::engine::WallEdge* walls, int wallCount,
                         const Vector* bodies, int bodyCount) {
    Vector hit;
    if (walls && acc::engine::SegmentCrossesWalkmesh(
            walls, wallCount, pos, b, hit)) {
        return "wall";
    }
    const float sdx = b.x - pos.x;
    const float sdy = b.y - pos.y;
    const float lenSq = sdx * sdx + sdy * sdy;
    if (lenSq <= 1e-6f) return nullptr;
    for (int j = 0; j < bodyCount; ++j) {
        float t = ((bodies[j].x - pos.x) * sdx +
                   (bodies[j].y - pos.y) * sdy) / lenSq;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
        const float cx = pos.x + sdx * t;
        const float cy = pos.y + sdy * t;
        const float ddx2 = bodies[j].x - cx;
        const float ddy2 = bodies[j].y - cy;
        if (ddx2 * ddx2 + ddy2 * ddy2 < kBodyClearanceMetersSq) return "body";
    }
    return nullptr;
}

// "Free directions: North, East", or the all-blocked line.
void FormatFreeDirections(const bool* freeDir, char* buf, size_t bufSize) {
    int written = 0;
    int freeCount = 0;
    for (int i = 0; i < 8; ++i) {
        if (!freeDir[i]) continue;
        const char* word = acc::strings::Get(kProbeDirs[i].strId);
        if (freeCount == 0) {
            written = snprintf(buf, bufSize, "%s: %s",
                               acc::strings::Get(
                                   acc::strings::Id::StuckFreeDirsPrefix),
                               word);
        } else if (written > 0 && written < static_cast<int>(bufSize)) {
            const int rem = static_cast<int>(bufSize) - written;
            const int add = snprintf(buf + written, rem, ", %s", word);
            if (add > 0) written += add;
        }
        ++freeCount;
    }
    if (freeCount == 0) {
        snprintf(buf, bufSize, "%s",
                 acc::strings::Get(acc::strings::Id::StuckAllBlocked));
    }
}

void RunStuckProbe(const Vector& pos) {
    // Walls — borrowed pointer into the change-detector's per-area cache.
    const acc::engine::WallEdge* walls = nullptr;
    int wallCount = 0;
    if (!acc::spatial::wall_surfaces::GetCachedWalls(walls, wallCount)) {
        walls = nullptr;
        wallCount = 0;
    }

    Vector bodies[kMaxNearbyBodies];
    const int bodyCount = CollectNearbyBodies(pos, bodies, kMaxNearbyBodies);

    bool        freeDir[8] = { false, false, false, false, false, false, false, false };
    const char* blockBy[8] = { "",    "",    "",    "",    "",    "",    "",    ""    };
    for (int i = 0; i < 8; ++i) {
        Vector b = pos;
        b.x += kProbeDirs[i].x * kProbeDistanceMeters;
        b.y += kProbeDirs[i].y * kProbeDistanceMeters;
        // Z stays — SegmentCrossesWalkmesh is planar.
        const char* blocker =
            ProbeBlocker(pos, b, walls, wallCount, bodies, bodyCount);
        if (blocker) {
            blockBy[i] = blocker;
        } else {
            freeDir[i] = true;
        }
    }

    char buf[256];
    FormatFreeDirections(freeDir, buf, sizeof(buf));

    acclog::Write("StuckProbe",
        "fire pos=(%.2f,%.2f) walls=%d bodies=%d "
        "N=%d/%s NE=%d/%s E=%d/%s SE=%d/%s S=%d/%s SW=%d/%s W=%d/%s NW=%d/%s "
        "-> [%s]",
        pos.x, pos.y, wallCount, bodyCount,
        freeDir[0] ? 1 : 0, blockBy[0],
        freeDir[1] ? 1 : 0, blockBy[1],
        freeDir[2] ? 1 : 0, blockBy[2],
        freeDir[3] ? 1 : 0, blockBy[3],
        freeDir[4] ? 1 : 0, blockBy[4],
        freeDir[5] ? 1 : 0, blockBy[5],
        freeDir[6] ? 1 : 0, blockBy[6],
        freeDir[7] ? 1 : 0, blockBy[7],
        buf);

    acc::speech::Speak(buf, /*interrupt=*/true);
}

}  // namespace

// Fires when walking but oscillating: cumulative path length to net
// displacement ratio is high over a 4s window. Reuses RunStuckProbe; the
// "circling detected" log line distinguishes the trigger path.
void TickCirclingAnnounce(const Vector& pos, uint64_t now_ms, bool walking) {
    if (!walking) {
        g_circling_anchor_ms = 0;
        g_circling_path_len  = 0.0f;
        g_circling_announced = false;
        return;
    }

    if (g_circling_anchor_ms == 0) {
        g_circling_anchor_pos = pos;
        g_circling_anchor_ms  = now_ms;
        g_circling_prev_pos   = pos;
        g_circling_path_len   = 0.0f;
        return;
    }

    const float dx_step = pos.x - g_circling_prev_pos.x;
    const float dy_step = pos.y - g_circling_prev_pos.y;
    g_circling_path_len += sqrtf(dx_step * dx_step + dy_step * dy_step);
    g_circling_prev_pos = pos;

    if ((now_ms - g_circling_anchor_ms) < kCirclingWindowMs) {
        return;
    }

    // Window elapsed — evaluate.
    const float dx_net = pos.x - g_circling_anchor_pos.x;
    const float dy_net = pos.y - g_circling_anchor_pos.y;
    const float netDist = sqrtf(dx_net * dx_net + dy_net * dy_net);
    const float ratio   = (netDist > 0.01f)
        ? (g_circling_path_len / netDist) : g_circling_path_len;

    const bool circling = (g_circling_path_len >= kCirclingMinPathMeters) &&
                          (netDist <= kCirclingMaxNetMeters) &&
                          (g_circling_path_len >=
                           netDist * kCirclingPathRatio);

    if (circling) {
        if (!g_circling_announced) {
            acclog::Write("StuckProbe",
                "circling detected — path=%.2fm net=%.2fm ratio=%.2f "
                "window_ms=%llu",
                g_circling_path_len, netDist, ratio,
                static_cast<unsigned long long>(
                    now_ms - g_circling_anchor_ms));
            RunStuckProbe(pos);
            g_circling_announced = true;
        }
        // Keep the anchor — re-arm only when the user escapes.
    } else {
        // Real progress — re-anchor + re-arm.
        g_circling_anchor_pos = pos;
        g_circling_anchor_ms  = now_ms;
        g_circling_path_len   = 0.0f;
        g_circling_announced  = false;
    }
}

void TickStuckAnnounce(const Vector& pos, uint64_t now_ms, bool walking) {
    if (!walking) {
        g_progress_checkpoint_ms = 0;
        g_stuck_announced = false;
        return;
    }

    if (g_progress_checkpoint_ms == 0) {
        g_progress_checkpoint_pos = pos;
        g_progress_checkpoint_ms  = now_ms;
        return;
    }

    if ((now_ms - g_progress_checkpoint_ms) < kStuckWindowMs) {
        return;
    }

    const float distSq = DistXYSq(pos, g_progress_checkpoint_pos);

    if (distSq >= kAnnounceDisplacementMetersSq) {
        g_progress_checkpoint_pos = pos;
        g_progress_checkpoint_ms  = now_ms;
        g_stuck_announced = false;
        return;
    }

    // Latch — re-arms via the displacement branch above.
    if (!g_stuck_announced) {
        RunStuckProbe(pos);
        g_stuck_announced = true;
    }
}

void ResetProbeTriggers() {
    g_progress_checkpoint_ms = 0;
    g_stuck_announced = false;
    g_circling_anchor_ms = 0;
    g_circling_path_len  = 0.0f;
    g_circling_announced = false;
}

}  // namespace acc::audio::footstep_suppress
