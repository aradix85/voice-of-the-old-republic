// Wall cache + surface clustering, rebuilt when the player enters an area.
// Room shapes, the Q/E wall test, route smoothing and the view-mode cursor
// all read it.
//
// The walkmesh stores wall geometry as many short edges (Endar Spire
// corridors are ~0.7-1m per edge). Adjacent collinear edges are clustered
// into "surfaces" at area-load time, so consumers can reason about one
// physical wall instead of its fragments.
//
// Cluster rule: two edges merge if they share an endpoint (within
// kEndpointTolMeters) AND their direction vectors agree within
// kSurfaceCollinearityCosThreshold. Cross-room merging is intentional —
// KOTOR's "room" is an internal .lyt segmentation and a physical wall
// regularly spans multiple rooms. The geometric tests decide alone.
//
// Each clustered surface is also reduced to a single straight segment
// (two extreme endpoints + unit direction + length) so room_topology
// doesn't have to re-merge the edge soup.

#pragma once

#include "engine_offsets.h"
#include "engine_area.h"

namespace acc::spatial::wall_surfaces {

// Sized for Endar Spire (405-908 perimeter edges observed); open-
// environment areas (Manaan, Korriban) plausibly run higher. Overflow
// is logged once per area-change so we can size up if a real area
// exceeds it.
constexpr int kMaxWallEdges    = 4096;
constexpr int kMaxWallSurfaces = 1024;

// One wall surface = chain of collinear endpoint-sharing edges
// clustered at area-load via union-find.
//
// a/b = the two extreme endpoints (only ones used by exactly one edge).
// Consumed by room_topology to skip a redundant merge pass.
//
// edge_count == 0 flags a degenerate surface (closed loop or zero free
// endpoints) — GetSurfaceDesc returns false for those.
struct WallSurfaceDesc {
    Vector a;
    Vector b;
    float  dir_x;
    float  dir_y;
    float  length;
    int    edge_count;
};

// Main thread, once per frame: rebuilds the caches when the player is in a
// new area. Without a player (menus, chargen, before spawn) the area is
// forgotten, so the next area load rebuilds.
void Tick();

// The cached perimeter edges, portal seams filtered. False before the first
// rebuild or for an area without walls. Borrowed: valid until the next
// rebuild.
bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount);

// Rebuild for this area. Calls BuildAreaWallCache, runs the union-find
// clustering pass, builds per-surface descriptors. Logs counts +
// overflow + descriptor anomalies. Silent on null area (calls Clear()
// instead).
void RebuildForArea(void* area);

// Reset to "no area" state. Wall and surface counts go to zero;
// subsequent accessor reads return false / empty.
void Clear();

// Borrowed pointer into static storage, valid until the next
// RebuildForArea or Clear. Don't retain across area transitions.
// Returns nullptr before first RebuildForArea.
const acc::engine::WallEdge* GetWallBuffer();

// Parallel array to GetWallBuffer(): element i is the surface index for
// wall edge i. -1 if pre-clustering or kMaxWallSurfaces overflowed and
// the edge ended up in the degraded bucket. nullptr before first
// RebuildForArea.
const int* GetEdgeSurfaceIdBuffer();

// 0 before first RebuildForArea.
int GetWallCount();

// 0 before first RebuildForArea.
int GetSurfaceCount();

// False on out-of-range, before clustering runs, or for surfaces that
// couldn't reduce to a single straight segment (closed-loop / branching
// edge cases flagged with edge_count == 0).
bool GetSurfaceDesc(int idx, WallSurfaceDesc& outDesc);

// 2D segment-vs-surface intersection. Portal-seam phantoms absorbed by
// clustering are invisible here, so a cursor stops at real walls, not on
// absorbed fragments.
bool SegmentCrossesSurface(const Vector& a, const Vector& b,
                           Vector& outHitPoint);

// Sized like the global tri-edge index in engine_area_walls (16384 edges
// ≈ 5500 faces for K1's densest observed area, ~3× headroom over the
// Peragus builds). 8192 × 36 B ≈ 290 KB static. Overflow logged once
// per area-change.
constexpr int kMaxFloorTris = 8192;

// Floor heights of the walkmesh triangles containing (x,y), one entry
// per stacked storey (bridge over tunnel), exact on ramps via plane
// interpolation. Fills outZ ascending, returns the count (0 when the
// point is off-mesh or before the first RebuildForArea). Primary source
// for wall_probe::ResolveProbeZ — the walls-only height vote is its
// fallback.
int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut);

}  // namespace acc::spatial::wall_surfaces
