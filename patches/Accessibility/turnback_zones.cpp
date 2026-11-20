// Zones the mod's own path stays out of (turnback_zones.h).

#include "turnback_zones.h"

namespace acc::guidance::turnback {

namespace {

struct Zone {
    Vector poly[kMaxVerts];
    int    count    = 0;
    bool   standing = false;
};

Zone        g_zones[kMaxZones];
int         g_count = 0;
const void* g_area  = nullptr;

bool Holds(const Zone& z, const Vector& p) {
    return acc::engine::PointInPolygonXY(z.poly, z.count, p.x, p.y);
}

bool AddZone(const Vector* poly, int count, bool standing) {
    if (!poly || count < 3 || count > kMaxVerts || g_count == kMaxZones) return false;
    // The same trigger hands out the same corners in the same order.
    for (int i = 0; i < g_count; ++i) {
        if (g_zones[i].count == count &&
            DistXYSq(g_zones[i].poly[0], poly[0]) < 1e-4f) {
            return false;
        }
    }
    Zone& z = g_zones[g_count++];
    z.count    = count;
    z.standing = standing;
    for (int i = 0; i < count; ++i) z.poly[i] = poly[i];
    return true;
}

void Outlines(const Vector& a, const Vector& b, bool standingOnly,
              std::vector<acc::engine::WallEdge>& out) {
    for (int i = 0; i < g_count; ++i) {
        const Zone& z = g_zones[i];
        if (standingOnly && !z.standing) continue;
        if (Holds(z, a) || Holds(z, b)) continue;
        for (int v = 0; v < z.count; ++v) {
            out.push_back(acc::engine::WallEdge{z.poly[v], z.poly[(v + 1) % z.count],
                                                -1, -1});
        }
    }
}

bool CrossesOutlines(const Vector& a, const Vector& b, bool standingOnly) {
    std::vector<acc::engine::WallEdge> walls;
    Outlines(a, b, standingOnly, walls);
    Vector hit{};
    return acc::engine::SegmentCrossesWalkmesh(walls.data(),
                                               static_cast<int>(walls.size()),
                                               a, b, hit, /*ignoreZ=*/true);
}

}  // namespace

void ForArea(const void* area) {
    if (area == g_area) return;
    g_area  = area;
    g_count = 0;
}

bool Add(const Vector* poly, int count) {
    return AddZone(poly, count, /*standing=*/false);
}

void ClearStanding() {
    int kept = 0;
    for (int i = 0; i < g_count; ++i) {
        if (g_zones[i].standing) continue;
        if (kept != i) g_zones[kept] = g_zones[i];
        ++kept;
    }
    g_count = kept;
}

bool AddStanding(const Vector* poly, int count) {
    return AddZone(poly, count, /*standing=*/true);
}

void AppendWalls(const Vector& a, const Vector& b,
                 std::vector<acc::engine::WallEdge>& out) {
    Outlines(a, b, /*standingOnly=*/false, out);
}

bool Crosses(const Vector& a, const Vector& b) {
    return CrossesOutlines(a, b, /*standingOnly=*/false);
}

bool CrossesStanding(const Vector& a, const Vector& b) {
    return CrossesOutlines(a, b, /*standingOnly=*/true);
}

}  // namespace acc::guidance::turnback
