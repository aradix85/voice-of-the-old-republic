// The wall and floor caches of the current area, rebuilt when the player
// enters one. Room shapes, the Q/E wall test, route smoothing and the
// stuck probe read the walls; the wall probe reads the floor heights.

#pragma once

#include "engine_area.h"
#include "engine_offsets.h"

namespace acc::spatial::wall_surfaces {

// Sized for the Endar Spire (405-908 perimeter edges observed); open areas
// plausibly run higher. Overflow is logged per area change.
constexpr int kMaxWallEdges = 4096;

// Main thread, once per frame: rebuilds the caches when the player is in a
// new area. Without a player (menus, chargen, before spawn) the area is
// forgotten, so the next area load rebuilds.
void Tick();

// The cached perimeter edges, portal seams filtered. False before the first
// rebuild or for an area without walls. Borrowed: valid until the next
// rebuild.
bool GetCachedWalls(const acc::engine::WallEdge*& outBuf, int& outCount);

// Rebuilds both caches for this area; a null area clears them.
void RebuildForArea(void* area);

// Back to "no area": the accessors return false or empty.
void Clear();

// About 5500 faces for KOTOR 1's densest observed area, with 3x headroom
// over the Peragus builds; 8192 x 36 B is about 290 KB static. Overflow is
// logged per area change.
constexpr int kMaxFloorTris = 8192;

// Floor heights of the walkmesh triangles containing (x,y), one entry per
// stacked storey (bridge over tunnel), exact on ramps via plane
// interpolation. Fills outZ ascending, returns the count (0 when the point
// is off-mesh or before the first RebuildForArea). Primary source for
// wall_probe::ResolveProbeZ; the walls-only height vote is its fallback.
int FloorZCandidatesAt(float x, float y, float* outZ, int maxOut);

}  // namespace acc::spatial::wall_surfaces
