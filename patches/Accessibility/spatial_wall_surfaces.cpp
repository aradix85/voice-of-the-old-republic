// The wall and floor caches of the current area — see spatial_wall_surfaces.h.

#include "spatial_wall_surfaces.h"

#include "engine_area.h"
#include "engine_player.h"
#include "log.h"

namespace acc::spatial::wall_surfaces {

namespace {

acc::engine::WallEdge g_walls[kMaxWallEdges];
int                   g_wall_count = 0;

acc::engine::FloorTri g_floor_tris[kMaxFloorTris];
int                   g_floor_tri_count = 0;

// The area the caches were built for; null until the player is in one.
void* g_cached_area = nullptr;

}  // namespace

void Tick() {
    Vector playerPos;
    if (!acc::engine::GetPlayerPosition(playerPos)) {
        g_cached_area = nullptr;
        return;
    }
    void* area = acc::engine::GetCurrentArea();
    if (!area || area == g_cached_area) return;
    RebuildForArea(area);
    g_cached_area = area;
}

bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount) {
    if (g_wall_count <= 0) return false;
    outBuf   = g_walls;
    outCount = g_wall_count;
    return true;
}

void Clear() {
    g_wall_count      = 0;
    g_floor_tri_count = 0;
}

void RebuildForArea(void* area) {
    if (!area) {
        Clear();
        return;
    }
    g_wall_count = acc::engine::BuildAreaWallCache(area, g_walls, kMaxWallEdges);
    const int wallsDiscovered = acc::engine::BuildAreaWallCache(area, nullptr, 0);

    const int trisDiscovered =
        acc::engine::BuildAreaFloorCache(area, g_floor_tris, kMaxFloorTris);
    g_floor_tri_count =
        (trisDiscovered < kMaxFloorTris) ? trisDiscovered : kMaxFloorTris;
    if (trisDiscovered > kMaxFloorTris) {
        acclog::Write("WallSurfaces",
                      "floor-tri cache OVERFLOW: discovered=%d kMaxFloorTris=%d - "
                      "height lookups beyond the cap fall back to the wall vote",
                      trisDiscovered, kMaxFloorTris);
    }
    // The cached count is after the seam filter; the overflow test compares
    // the count before it against the buffer, because edges dropped during
    // the scan never reach the filter.
    acclog::Write("WallSurfaces",
                  "area change - walls cached=%d (seam-filtered from discovered=%d%s) "
                  "floor faces=%d areaPtr=%p",
                  g_wall_count, wallsDiscovered,
                  wallsDiscovered > kMaxWallEdges ? ", OVERFLOW: raise kMaxWallEdges"
                                                  : "",
                  g_floor_tri_count, area);
}

int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut) {
    if (!outZ || maxOut <= 0 || g_floor_tri_count <= 0) return 0;

    // Edge-inclusive tolerance: nav points authored exactly on a shared
    // triangle edge must not fall through the crack between two faces
    // whose sign tests both come out marginally negative.
    constexpr float kEdgeEps = 1e-3f;
    // Two stacked candidates closer than this are the same storey seen
    // through overlapping faces (seam authoring) — keep the first.
    constexpr float kSameStoreyM = 1.0f;

    int found = 0;
    for (int i = 0; i < g_floor_tri_count; ++i) {
        const acc::engine::FloorTri& t = g_floor_tris[i];
        // 2D sign test: (x,y) is inside when it sits on the same side of
        // all three directed edges (either orientation).
        float d1 = (x - t.b.x) * (t.a.y - t.b.y) - (t.a.x - t.b.x) * (y - t.b.y);
        float d2 = (x - t.c.x) * (t.b.y - t.c.y) - (t.b.x - t.c.x) * (y - t.c.y);
        float d3 = (x - t.a.x) * (t.c.y - t.a.y) - (t.c.x - t.a.x) * (y - t.a.y);
        bool hasNeg = (d1 < -kEdgeEps) || (d2 < -kEdgeEps) || (d3 < -kEdgeEps);
        bool hasPos = (d1 > kEdgeEps) || (d2 > kEdgeEps) || (d3 > kEdgeEps);
        if (hasNeg && hasPos) continue;

        // Height of the triangle's plane at (x,y) via barycentric
        // weights. Degenerate (2D-collinear) faces are skipped — a .wok
        // floor face always has real area, so these are data noise.
        float denom = (t.b.y - t.c.y) * (t.a.x - t.c.x) +
                      (t.c.x - t.b.x) * (t.a.y - t.c.y);
        if (denom > -1e-6f && denom < 1e-6f) continue;
        float wa = ((t.b.y - t.c.y) * (x - t.c.x) +
                    (t.c.x - t.b.x) * (y - t.c.y)) / denom;
        float wb = ((t.c.y - t.a.y) * (x - t.c.x) +
                    (t.a.x - t.c.x) * (y - t.c.y)) / denom;
        float z = wa * t.a.z + wb * t.b.z + (1.0f - wa - wb) * t.c.z;

        bool dup = false;
        for (int k = 0; k < found; ++k) {
            float dz = outZ[k] - z;
            if (dz < 0.0f) dz = -dz;
            if (dz < kSameStoreyM) { dup = true; break; }
        }
        if (dup) continue;
        if (found < maxOut) {
            outZ[found++] = z;
        }
    }

    // Ascending order (found is tiny — 1-3 entries).
    for (int i = 1; i < found; ++i) {
        float v = outZ[i];
        int j = i - 1;
        while (j >= 0 && outZ[j] > v) { outZ[j + 1] = outZ[j]; --j; }
        outZ[j + 1] = v;
    }
    return found;
}

}  // namespace acc::spatial::wall_surfaces
