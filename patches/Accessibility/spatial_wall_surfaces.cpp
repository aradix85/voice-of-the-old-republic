#include "spatial_wall_surfaces.h"

#include <cmath>

#include "engine_area.h"
#include "engine_player.h"
#include "log.h"

namespace acc::spatial::wall_surfaces {

namespace {

// --- Cache + clustering state -----------------------------------------

acc::engine::WallEdge g_walls[kMaxWallEdges];
int                   g_wall_count = 0;

// Per-edge surface-id mapping. Filled by the area-load clustering pass.
// -1 = "edge not assigned" (only seen briefly during clustering or if
// kMaxWallSurfaces overflowed).
int g_edge_surface_id[kMaxWallEdges];

// Number of distinct surfaces clustered for the current area.
int g_surface_count = 0;

// Per-surface segment descriptor built right after clustering.
// edge_count == 0 flags a degenerate surface (closed loop or zero
// free endpoints) — GetSurfaceDesc returns false for those.
WallSurfaceDesc g_surface_descriptors[kMaxWallSurfaces];

// --- Clustering constants ---------------------------------------------

// cos(15°) ≈ 0.966 — direction vectors that disagree by more than 15°
// are different surfaces. Generous enough to absorb walkmesh dicing
// noise on smooth corridor walls, tight enough to keep L-junctions
// distinct (90° → cos=0, well below threshold).
constexpr float kSurfaceCollinearityCosThreshold = 0.966f;

// Endpoint coincidence tolerance — 5cm. Walkmesh world-space transforms
// can introduce sub-mm float noise; 5cm is well below any realistic
// edge spacing in KOTOR's hand-authored layouts.
constexpr float kEndpointTolMeters  = 0.05f;
constexpr float kEndpointTolSquared = kEndpointTolMeters * kEndpointTolMeters;

// --- Geometry helpers --------------------------------------------------

// Clustering measures in XY (DistXYSq): walkmesh edges are flat per room,
// and a small Z jitter on a seam would otherwise keep endpoints apart.

// Returns true if the two edges should belong to the same wall surface:
// they share an endpoint (within kEndpointTolMeters) AND their direction
// vectors are roughly collinear (cosine within
// kSurfaceCollinearityCosThreshold). Direction is taken in XY only;
// walkmesh edges within a single room are co-planar enough that
// vertical components add noise without information.
bool EdgesAreSameSurface(const acc::engine::WallEdge& e1,
                         const acc::engine::WallEdge& e2) {
    bool share_aa = DistXYSq(e1.a, e2.a) <= kEndpointTolSquared;
    bool share_ab = DistXYSq(e1.a, e2.b) <= kEndpointTolSquared;
    bool share_ba = DistXYSq(e1.b, e2.a) <= kEndpointTolSquared;
    bool share_bb = DistXYSq(e1.b, e2.b) <= kEndpointTolSquared;
    if (!(share_aa || share_ab || share_ba || share_bb)) return false;

    float d1x = e1.b.x - e1.a.x;
    float d1y = e1.b.y - e1.a.y;
    float d2x = e2.b.x - e2.a.x;
    float d2y = e2.b.y - e2.a.y;
    float len1 = std::sqrt(d1x * d1x + d1y * d1y);
    float len2 = std::sqrt(d2x * d2x + d2y * d2y);
    if (len1 < 1e-4f || len2 < 1e-4f) return true;  // degenerate, accept
    float cos_unsigned =
        std::fabs((d1x * d2x + d1y * d2y) / (len1 * len2));
    return cos_unsigned >= kSurfaceCollinearityCosThreshold;
}

// --- Union-find over edges (small-N implementation) -------------------
//
// Iterative path compression with union-by-rank. We use it once per
// area-load to cluster edges into surfaces; runtime is O(N² α(N)) in
// the worst case (we do N² pair tests; the union/find calls are
// effectively constant). For 900 edges that's ~800k pair tests + O(N)
// union-find ops, which finishes in well under 100ms — runs once per
// zone, never on a hot path.

int g_uf_parent[kMaxWallEdges];
int g_uf_rank[kMaxWallEdges];

int UfFind(int i) {
    while (g_uf_parent[i] != i) {
        g_uf_parent[i] = g_uf_parent[g_uf_parent[i]];  // path compress
        i = g_uf_parent[i];
    }
    return i;
}

void UfUnion(int i, int j) {
    int ri = UfFind(i);
    int rj = UfFind(j);
    if (ri == rj) return;
    if (g_uf_rank[ri] < g_uf_rank[rj]) {
        g_uf_parent[ri] = rj;
    } else if (g_uf_rank[ri] > g_uf_rank[rj]) {
        g_uf_parent[rj] = ri;
    } else {
        g_uf_parent[rj] = ri;
        ++g_uf_rank[ri];
    }
}

// Cluster the cached wall edges (g_walls[0..g_wall_count-1]) into
// connected-and-collinear surfaces. Fills g_edge_surface_id[] and sets
// g_surface_count. Each unique cluster representative is assigned a
// dense surface index in [0, g_surface_count). Surfaces beyond
// kMaxWallSurfaces are coalesced into surface index 0 (degraded but
// safe; logs a warning).
void ClusterEdgesIntoSurfaces() {
    g_surface_count = 0;
    for (int i = 0; i < g_wall_count; ++i) {
        g_uf_parent[i] = i;
        g_uf_rank[i]   = 0;
        g_edge_surface_id[i] = -1;
    }
    // Cross-room merging is intentional. KOTOR's "room" is an internal
    // .lyt segmentation of an area (not a literal room), and a single
    // physical corridor wall regularly spans multiple rooms. Refusing
    // to cluster across room_id leaves long walls split into per-room
    // fragments, each firing its own threshold crossings. We let the
    // geometric tests (shared endpoint + collinearity) decide alone.
    for (int i = 0; i < g_wall_count; ++i) {
        for (int j = i + 1; j < g_wall_count; ++j) {
            if (EdgesAreSameSurface(g_walls[i], g_walls[j])) {
                UfUnion(i, j);
            }
        }
    }
    // Densify: walk roots in iteration order, assigning surface indices
    // 0..N-1.
    int root_to_surface[kMaxWallEdges];
    for (int i = 0; i < g_wall_count; ++i) root_to_surface[i] = -1;
    int overflow_collisions = 0;
    for (int i = 0; i < g_wall_count; ++i) {
        int r = UfFind(i);
        if (root_to_surface[r] < 0) {
            if (g_surface_count < kMaxWallSurfaces) {
                root_to_surface[r] = g_surface_count++;
            } else {
                root_to_surface[r] = 0;  // degrade to bucket 0
                ++overflow_collisions;
            }
        }
        g_edge_surface_id[i] = root_to_surface[r];
    }
    if (overflow_collisions > 0) {
        acclog::Write("WallSurfaces", "surface overflow — %d edges collapsed to "
            "bucket 0 (raise kMaxWallSurfaces above %d)",
            overflow_collisions, kMaxWallSurfaces);
    }
    acclog::Write("WallSurfaces", "clustered %d edges into %d surfaces",
        g_wall_count, g_surface_count);
}

// Reduces each surface (a chain of collinear edges sharing endpoints) to
// one straight segment a-b: its two ends are the only endpoints used by
// exactly one edge. A closed loop, a fork, or a chain with more than
// kMaxPointsPerSurface distinct endpoints has no such pair and keeps
// edge_count 0, which SegmentCrossesSurface skips.
constexpr int kMaxPointsPerSurface = 64;

void BuildSurfaceDescriptors() {
    struct EndpointCount {
        Vector pos;
        int    count;
    };
    int skipped = 0;
    for (int s = 0; s < g_surface_count; ++s) {
        g_surface_descriptors[s] = {};

        EndpointCount points[kMaxPointsPerSurface];
        int  pointCount = 0;
        int  edgeCount  = 0;
        bool overflow   = false;
        auto bumpEndpoint = [&](const Vector& p) {
            for (int k = 0; k < pointCount; ++k) {
                if (DistXYSq(points[k].pos, p) <= kEndpointTolSquared) {
                    ++points[k].count;
                    return;
                }
            }
            if (pointCount >= kMaxPointsPerSurface) {
                overflow = true;
                return;
            }
            points[pointCount++] = {p, 1};
        };
        for (int i = 0; i < g_wall_count; ++i) {
            if (g_edge_surface_id[i] != s) continue;
            ++edgeCount;
            bumpEndpoint(g_walls[i].a);
            bumpEndpoint(g_walls[i].b);
        }

        int    freeCount = 0;
        Vector freeEnds[2] = {};
        for (int k = 0; k < pointCount; ++k) {
            if (points[k].count != 1) continue;
            if (freeCount < 2) freeEnds[freeCount] = points[k].pos;
            ++freeCount;
        }
        if (freeCount != 2 || overflow) {
            ++skipped;
            continue;
        }
        WallSurfaceDesc& d = g_surface_descriptors[s];
        d.a = freeEnds[0];
        d.b = freeEnds[1];
        d.length = DistXY(d.a, d.b);
        if (d.length > 1e-6f) {
            d.dir_x = (d.b.x - d.a.x) / d.length;
            d.dir_y = (d.b.y - d.a.y) / d.length;
        }
        d.edge_count = edgeCount;
    }
    if (skipped > 0) {
        acclog::Write("WallSurfaces", "%d of %d surfaces are loops or forks without a "
                      "single segment", skipped, g_surface_count);
    }
}

// Floor-triangle cache, rebuilt alongside the wall cache. Storage for
// FloorZCandidatesAt (see header).
acc::engine::FloorTri g_floor_tris[kMaxFloorTris];
int g_floor_tri_count = 0;

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
    g_surface_count   = 0;
    g_floor_tri_count = 0;
}

void RebuildForArea(void* area) {
    if (!area) {
        Clear();
        return;
    }
    g_wall_count = acc::engine::BuildAreaWallCache(area, g_walls, kMaxWallEdges);
    int totalDiscovered = acc::engine::BuildAreaWallCache(area, nullptr, 0);
    bool overflow = (totalDiscovered > kMaxWallEdges);

    int triDiscovered = acc::engine::BuildAreaFloorCache(
        area, g_floor_tris, kMaxFloorTris);
    g_floor_tri_count = (triDiscovered < kMaxFloorTris) ? triDiscovered
                                                        : kMaxFloorTris;
    if (triDiscovered > kMaxFloorTris) {
        acclog::Write("WallSurfaces",
            "floor-tri cache OVERFLOW: discovered=%d kMaxFloorTris=%d — "
            "height lookups beyond the cap fall back to the wall vote",
            triDiscovered, kMaxFloorTris);
    } else {
        acclog::Write("WallSurfaces", "floor-tri cache: %d faces",
                      g_floor_tri_count);
    }

    ClusterEdgesIntoSurfaces();
    BuildSurfaceDescriptors();

    // Telemetry: g_wall_count is post-seam-filter (real walls); the
    // overflow check uses the pre-filter discovered count vs buffer size,
    // because edges dropped during the scan never reach the seam filter.
    if (overflow) {
        acclog::Write("WallSurfaces", "area change — walls cached=%d "
            "(discovered=%d, OVERFLOW pre-seam-filter; increase "
            "kMaxWallEdges) areaPtr=%p",
            g_wall_count, totalDiscovered, area);
    } else {
        acclog::Write("WallSurfaces", "area change — walls cached=%d "
            "(seam-filtered from discovered=%d) areaPtr=%p",
            g_wall_count, totalDiscovered, area);
    }
}

bool SegmentCrossesSurface(const Vector& a, const Vector& b,
                           Vector& outHitPoint) {
    // Movement direction in 2D. If both deltas are ~0 the segment isn't
    // actually moving and there's nothing to test (and the parametric
    // formula below would divide by zero).
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    if (abx * abx + aby * aby < 1e-10f) return false;

    bool   anyHit  = false;
    float  bestT   = 1e30f;
    Vector bestHit = a;

    // Iterate over clustered surfaces (the same representation the audio
    // wall-cue system reads). Each surface is a straight line segment
    // from `a` to `b` covering one or more collinear+connected raw
    // edges; portal seams that happen to be collinear with a real wall
    // are absorbed into that wall's surface during clustering, so this
    // test ignores phantom edges by construction. edge_count == 0
    // flags a degenerate descriptor and is skipped.
    for (int i = 0; i < g_surface_count; ++i) {
        const WallSurfaceDesc& s = g_surface_descriptors[i];
        if (s.edge_count <= 0) continue;
        float cdx = s.b.x - s.a.x;
        float cdy = s.b.y - s.a.y;

        // 2D segment-segment intersection in XY.
        //   a + t*(b - a) == s.a + u*(s.b - s.a)
        float denom = abx * cdy - aby * cdx;
        if (denom > -1e-8f && denom < 1e-8f) continue;

        float dx = s.a.x - a.x;
        float dy = s.a.y - a.y;
        float t  = (dx * cdy - dy * cdx) / denom;
        float u  = (dx * aby - dy * abx) / denom;

        if (t < 0.0f || t > 1.0f) continue;
        if (u < 0.0f || u > 1.0f) continue;

        if (t < bestT) {
            bestT     = t;
            bestHit.x = a.x + t * abx;
            bestHit.y = a.y + t * aby;
            bestHit.z = a.z + t * (b.z - a.z);
            anyHit    = true;
        }
    }

    if (anyHit) outHitPoint = bestHit;
    return anyHit;
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
