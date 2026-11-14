// Engine-free geometry over the walkmesh wall cache: the wall edge and
// floor triangle records engine_area_walls.cpp extracts, the two filters
// that turn its raw perimeter edges into real walls, and the
// segment-vs-wall test the guidance, topology and cursor code use.

#pragma once

#include "engine_offsets_types.h"

namespace acc::engine {

// One perimeter walkmesh edge: a triangle side with no neighbour
// (SurfaceMeshAdjacency index -1), in world space.
struct WallEdge {
    Vector a;
    Vector b;
    int    room_id;       // CSWSArea-local room index
    int    material_id;   // surfacemat.2da row of the face this edge bounds
};

// One walkmesh floor triangle in world space. The .wok mesh is a 2.5D floor
// with no ceilings, so the triangle under a point is the floor there and a
// plane through its corners gives the height, ramps included.
struct FloorTri {
    Vector a;
    Vector b;
    Vector c;
    int    material_id;   // surfacemat.2da row, -1 when the mesh has none
};

// True for a surfacemat.2da row a creature can stand on (its "walk" column;
// the same rows in both games, bar KOTOR 2's walkable BottomlessPit, which
// nothing here should send anyone onto).
bool IsWalkableMaterial(int materialId);

// The point of the walkable floor nearest to p on the floor plan, within maxM
// and within kWallCrossZToleranceM of p's height: p itself when it stands on
// that floor, else a point on the floor's edge. outPoint.z is the floor's.
// False when no walkable floor is that near.
bool NearestWalkableFloor(const FloorTri* tris, int triCount, const Vector& p,
                          float maxM, Vector& outPoint);

// Any triangle side of a room, for the portal filter.
struct TriEdge {
    Vector a;
    Vector b;
    int    room_id;
};

// Drops each wall edge that coincides (about 1 cm, either direction) with a
// triangle side of ANOTHER room. K1 joins rooms through portals, not
// triangle adjacency: a boundary side carries adjacency -1 even where the
// walkmesh continues into the next room, and that room's matching side may
// not. A wall is real only where no room has a triangle beyond it. Keeps the
// order; returns the kept count.
int DropPortalEdges(WallEdge* walls, int count, const TriEdge* sides, int sideCount);

// Drops the second of each same-room pair whose floor-plan footprints match
// and that share at least one 3D endpoint: one edge on two non-manifold
// faces, or a step's flat foot edge plus its slanted face edge (they share
// the foot). Walls of two floors at the same XY share no endpoint and both
// stay. Swap-removes, so the order changes; returns the kept count.
int DropSameRoomDuplicates(WallEdge* walls, int count);

// How far (metres) a wall's z at the crossing may sit from the ray's z and
// still block. 2 m clears step and slope authoring and excludes real
// stacked decks: every over-block a nav-graph crosscheck found was an
// elevated ray against a ground-floor wall. Shared with
// room_topology::LogNavWallCrossings.
constexpr float kWallCrossZToleranceM = 2.0f;

// The closest wall that a→b crosses in the floor plan (smallest t in
// [0,1]), whose z at the crossing is within kWallCrossZToleranceM of the
// ray's. ignoreZ drops that guard for callers without trustworthy z (the
// waypoint smoother's 2D nav nodes): a pure 2D test can only over-block,
// never route through a real wall. False for no walls or a == b.
bool SegmentCrossesWalkmesh(const WallEdge* walls,
                            int wallCount,
                            const Vector& a,
                            const Vector& b,
                            Vector& outHitPoint,
                            bool ignoreZ = false);

}  // namespace acc::engine
