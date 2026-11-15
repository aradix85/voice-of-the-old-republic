// Turn-back zones (turnback_zones.h).

#include "turnback_zones.h"

namespace acc::guidance::turnback {

namespace {

struct Zone {
    Vector poly[kMaxVerts];
    int    count = 0;
};

Zone        g_zones[kMaxZones];
int         g_count = 0;
const void* g_area  = nullptr;

bool Holds(const Zone& z, const Vector& p) {
    return acc::engine::PointInPolygonXY(z.poly, z.count, p.x, p.y);
}

}  // namespace

void ForArea(const void* area) {
    if (area == g_area) return;
    g_area  = area;
    g_count = 0;
}

bool Add(const Vector* poly, int count) {
    if (!poly || count < 3 || count > kMaxVerts || g_count == kMaxZones) return false;
    // The same trigger hands out the same corners in the same order.
    for (int i = 0; i < g_count; ++i) {
        if (g_zones[i].count == count &&
            DistXYSq(g_zones[i].poly[0], poly[0]) < 1e-4f) {
            return false;
        }
    }
    Zone& z = g_zones[g_count++];
    z.count = count;
    for (int i = 0; i < count; ++i) z.poly[i] = poly[i];
    return true;
}

void AppendWalls(const Vector& a, const Vector& b,
                 std::vector<acc::engine::WallEdge>& out) {
    for (int i = 0; i < g_count; ++i) {
        const Zone& z = g_zones[i];
        if (Holds(z, a) || Holds(z, b)) continue;
        for (int v = 0; v < z.count; ++v) {
            out.push_back(acc::engine::WallEdge{z.poly[v], z.poly[(v + 1) % z.count],
                                                -1, -1});
        }
    }
}

bool Crosses(const Vector& a, const Vector& b) {
    std::vector<acc::engine::WallEdge> walls;
    AppendWalls(a, b, walls);
    Vector hit{};
    return acc::engine::SegmentCrossesWalkmesh(walls.data(),
                                               static_cast<int>(walls.size()),
                                               a, b, hit, /*ignoreZ=*/true);
}

}  // namespace acc::guidance::turnback
