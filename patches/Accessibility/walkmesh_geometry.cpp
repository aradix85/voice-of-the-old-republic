// Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

#include "walkmesh_geometry.h"

namespace acc::engine {

bool SegmentCrossesWalkmesh(const WallEdge* walls,
                            int wallCount,
                            const Vector& a,
                            const Vector& b,
                            Vector& outHitPoint,
                            bool ignoreZ) {
    if (!walls || wallCount <= 0) return false;

    // Movement direction in 2D. abx/aby form the player segment; if both
    // are ~0 the cursor isn't actually moving and there's nothing to
    // test (avoids the divide-by-zero in the parametric formula below).
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    if (abx * abx + aby * aby < 1e-10f) return false;

    bool   anyHit       = false;
    float  bestT        = 1e30f;
    Vector bestHit      = a;

    for (int i = 0; i < wallCount; ++i) {
        const WallEdge& w = walls[i];
        float cdx = w.b.x - w.a.x;
        float cdy = w.b.y - w.a.y;

        // Standard 2D segment-segment intersection in the XY plane.
        // Solve for parametric t (along a→b) and u (along w.a→w.b):
        //   a + t * (b - a) == w.a + u * (w.b - w.a)
        // → denom = abx*cdy - aby*cdx (≈ 0 means the segments are
        //   parallel; treat as no hit — sliding-along is fine for our
        //   cursor scale).
        float denom = abx * cdy - aby * cdx;
        if (denom > -1e-8f && denom < 1e-8f) continue;

        float dx = w.a.x - a.x;
        float dy = w.a.y - a.y;
        float t  = (dx * cdy - dy * cdx) / denom;
        float u  = (dx * aby - dy * abx) / denom;

        if (t < 0.0f || t > 1.0f) continue;
        if (u < 0.0f || u > 1.0f) continue;

        // 3D guard: the XY test alone treats a wall on another floor as a
        // blocker whenever its 2D projection lands on a→b. Wall edges are
        // walkmesh floor-boundary edges, so their z tracks the floor they
        // bound; reject the hit when the wall edge sits more than a
        // same-floor tolerance from the ray at the crossing point. This is
        // the entire remaining over-block population (validated via the
        // nav-graph crosscheck) — every runtime false-block occurred at
        // elevated z against a ground-floor wall.
        //
        // Skipped when ignoreZ: callers with untrustworthy endpoint z (the
        // waypoint smoother feeds 2D nav nodes) must run pure 2D, or the
        // guard silently drops real same-floor walls when the ray's z is
        // bogus and routes the path straight through them.
        if (!ignoreZ) {
            float rayZ  = a.z   + t * (b.z   - a.z);
            float wallZ = w.a.z + u * (w.b.z - w.a.z);
            float dz = rayZ - wallZ;
            if (dz < 0.0f) dz = -dz;
            if (dz > kWallCrossZToleranceM) continue;
        }

        if (t < bestT) {
            bestT      = t;
            bestHit.x  = a.x + t * abx;
            bestHit.y  = a.y + t * aby;
            bestHit.z  = a.z + t * (b.z - a.z);
            anyHit     = true;
        }
    }

    if (anyHit) outHitPoint = bestHit;
    return anyHit;
}

}  // namespace acc::engine
