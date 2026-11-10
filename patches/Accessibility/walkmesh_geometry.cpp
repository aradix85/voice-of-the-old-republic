// Engine-free geometry over the walkmesh wall cache (walkmesh_geometry.h).

#include "walkmesh_geometry.h"

namespace acc::engine {

namespace {

// About 1 cm, squared: LocalToWorld's matrix math leaves "identical" edges
// of two rooms slightly apart.
constexpr float kSeamEpsSq = 1e-4f;

bool Same3D(const Vector& p, const Vector& q) { return DistXYZSq(p, q) < kSeamEpsSq; }
bool SameXY(const Vector& p, const Vector& q) { return DistXYSq(p, q) < kSeamEpsSq; }

// The same edge in either direction, under `same`.
template <typename Same>
bool SameEdge(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Same same) {
    return (same(a1, a2) && same(b1, b2)) || (same(a1, b2) && same(b1, a2));
}

bool SharesEndpoint3D(const WallEdge& p, const WallEdge& q) {
    return Same3D(p.a, q.a) || Same3D(p.a, q.b) || Same3D(p.b, q.a) || Same3D(p.b, q.b);
}

}  // namespace

int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount) {
    int kept = 0;
    for (int i = 0; i < count; ++i) {
        const WallEdge& w = walls[i];
        bool portal = false;
        for (int s = 0; s < sideCount && !portal; ++s) {
            portal = sides[s].room_id != w.room_id &&
                     SameEdge(w.a, w.b, sides[s].a, sides[s].b, Same3D);
        }
        if (!portal) walls[kept++] = w;
    }
    return kept;
}

int DropSameRoomDuplicates(WallEdge* walls, int count) {
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count;) {
            const WallEdge& p = walls[i];
            const WallEdge& q = walls[j];
            const bool duplicate = p.room_id == q.room_id &&
                                   SameEdge(p.a, p.b, q.a, q.b, SameXY) &&
                                   SharesEndpoint3D(p, q);
            if (duplicate) {
                walls[j] = walls[--count];
            } else {
                ++j;
            }
        }
    }
    return count;
}

bool SegmentCrossesWalkmesh(const WallEdge* walls,
                            int wallCount,
                            const Vector& a,
                            const Vector& b,
                            Vector& outHitPoint,
                            bool ignoreZ) {
    if (!walls || wallCount <= 0) return false;
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    if (abx * abx + aby * aby < 1e-10f) return false;  // not moving

    bool  anyHit = false;
    float bestT  = 1e30f;
    for (int i = 0; i < wallCount; ++i) {
        const WallEdge& w = walls[i];
        const float cdx = w.b.x - w.a.x;
        const float cdy = w.b.y - w.a.y;
        // a + t(b - a) == w.a + u(w.b - w.a); a zero denominator means
        // parallel, which counts as no hit (sliding along is fine).
        const float denom = abx * cdy - aby * cdx;
        if (denom > -1e-8f && denom < 1e-8f) continue;
        const float dx = w.a.x - a.x;
        const float dy = w.a.y - a.y;
        const float t  = (dx * cdy - dy * cdx) / denom;
        const float u  = (dx * aby - dy * abx) / denom;
        if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) continue;

        // Wall edges bound a floor, so their z follows it: a crossing more
        // than the tolerance above or below the ray is another floor's wall.
        if (!ignoreZ) {
            float dz = (a.z + t * (b.z - a.z)) - (w.a.z + u * (w.b.z - w.a.z));
            if (dz < 0.0f) dz = -dz;
            if (dz > kWallCrossZToleranceM) continue;
        }
        if (t < bestT) {
            bestT  = t;
            anyHit = true;
        }
    }
    if (anyHit) {
        outHitPoint = Vector{a.x + bestT * abx, a.y + bestT * aby, a.z + bestT * (b.z - a.z)};
    }
    return anyHit;
}

}  // namespace acc::engine
