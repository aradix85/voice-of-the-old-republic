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

// Twice the signed floor-plan area of a-b-p: positive when p lies left of a→b.
float Cross(const Vector& a, const Vector& b, const Vector& p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

// The point of the segment a-b nearest to p on the floor plan, at the
// segment's height there.
Vector NearestOnSegmentXY(const Vector& p, const Vector& a, const Vector& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }
    return Vector{a.x + t * dx, a.y + t * dy, a.z + t * (b.z - a.z)};
}

// The point of the triangle nearest to p on the floor plan, at the
// triangle's height there: p itself when it lies inside.
Vector NearestOnTriangleXY(const Vector& p, const FloorTri& t) {
    const float ab = Cross(t.a, t.b, p);
    const float bc = Cross(t.b, t.c, p);
    const float ca = Cross(t.c, t.a, p);
    const bool  left  = ab > 0.0f || bc > 0.0f || ca > 0.0f;
    const bool  right = ab < 0.0f || bc < 0.0f || ca < 0.0f;
    const float area  = Cross(t.a, t.b, t.c);
    if (!(left && right) && (area > 1e-6f || area < -1e-6f)) {
        return Vector{p.x, p.y, (bc * t.a.z + ca * t.b.z + ab * t.c.z) / area};
    }
    Vector best = NearestOnSegmentXY(p, t.a, t.b);
    const Vector others[] = {NearestOnSegmentXY(p, t.b, t.c), NearestOnSegmentXY(p, t.c, t.a)};
    for (const Vector& q : others) {
        if (DistXYSq(p, q) < DistXYSq(p, best)) best = q;
    }
    return best;
}

}  // namespace

bool IsWalkableMaterial(int materialId) {
    // Dirt, Grass, Stone, Wood, Water, Carpet, Metal, Puddles, Swamp, Mud,
    // Leaves, Door, Trigger.
    constexpr unsigned kWalkable = (1u << 1) | (1u << 3) | (1u << 4) | (1u << 5) | (1u << 6) |
                                   (1u << 9) | (1u << 10) | (1u << 11) | (1u << 12) |
                                   (1u << 13) | (1u << 14) | (1u << 18) | (1u << 30);
    return materialId >= 0 && materialId < 32 && ((kWalkable >> materialId) & 1u) != 0;
}

bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p,
                          float maxM, Vector& outPoint) {
    if (!tris) return false;
    float bestSq = maxM * maxM;
    bool  found  = false;
    for (int i = 0; i < triCount; ++i) {
        if (!IsWalkableMaterial(tris[i].material_id)) continue;
        const Vector q = NearestOnTriangleXY(p, tris[i]);
        const float dz = q.z - p.z;
        if (dz > kWallCrossZToleranceM || dz < -kWallCrossZToleranceM) continue;
        const float dSq = DistXYSq(p, q);
        if (found ? dSq >= bestSq : dSq > bestSq) continue;
        bestSq   = dSq;
        outPoint = q;
        found    = true;
    }
    return found;
}

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
