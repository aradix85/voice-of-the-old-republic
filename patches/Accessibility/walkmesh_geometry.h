// Engine-free geometry over the walkmesh wall cache: the wall edge and
// floor triangle records engine_area_walls.cpp extracts, and the
// segment-vs-wall test the guidance, topology and cursor code use.

#pragma once

#include "engine_offsets_types.h"

namespace acc::engine {

// One perimeter walkmesh edge — a triangle side that has no neighbour
// (`SurfaceMeshAdjacency.indices[e] == -1`) and therefore corresponds to a
// physical wall in the room. Endpoints are in WORLD space (LocalToWorld
// already applied at extraction time, so consumers do not need the room's
// CSWCollisionMesh anymore — the edge is self-contained).
struct WallEdge {
    Vector a;
    Vector b;
    int    room_id;       // CSWSArea-local room index (0..room_count-1)
    int    material_id;   // surfacemat.2da row id for the face this edge bounds
};

// One walkmesh floor triangle, all three corners in WORLD space
// (LocalToWorld applied at extraction, same as WallEdge). The .wok mesh
// is a 2.5D floor surface — no ceilings — so the triangle under an
// (x,y) point IS the floor there; a plane interpolation over the three
// corners gives the exact height even on ramps.
struct FloorTri {
    Vector a;
    Vector b;
    Vector c;
};

// Same-floor z tolerance for the segment-vs-wall test below (world units
// = metres). A wall edge whose z at the crossing point differs from the
// ray's z by more than this is on another floor and is not a blocker —
// see the 3D guard in SegmentCrossesWalkmesh. 2m clears step/slope edge
// authoring (~2.25m faces dedup elsewhere) while excluding genuine
// inter-deck stacks. Confirmed against the engine nav graph: a 3D-aware
// crosscheck found zero nav-edge/wall crossings across 6 diverse areas
// (wall cache is phantom-free), and every runtime over-block occurred at
// elevated z against ground-floor walls — i.e. pure 2D-projection noise
// this tolerance removes. Shared with room_topology::LogNavWallCrossings.
constexpr float kWallCrossZToleranceM = 2.0f;

// Segment-vs-perimeter test in the XY plane, with an optional 3D guard: a
// wall is only a blocker when its edge sits within kWallCrossZToleranceM of
// the ray at the crossing point. Without the guard, a wall on a different
// floor whose 2D projection happens to lie on a→b would falsely block
// (the engine's nav graph proves these are not real obstacles). Returns
// closest qualifying hit along a→b (smallest t in [0,1]).
//
// ignoreZ: when true, the z guard is skipped and the test is pure 2D.
// Callers whose endpoints carry no trustworthy z must set this — notably
// the waypoint smoother, which feeds 2D nav-graph nodes (no height field;
// see PathPoint layout in guidance_pathfind.h). A 2D test there fails safe:
// it can only over-block (keep a redundant waypoint), never miss a real
// wall and route through it. The default (false) keeps the guard for the
// room-shape/cursor consumers, where a phantom cross-floor wall would
// instead corrupt a spoken description.
//
// False on: null walls, wallCount<=0, a==b. Degenerate edges (a==b)
// skipped.
//
// Pulls from spatial::wall_surfaces::GetCachedWalls; this is the
// consumer surface, not the build path.
bool SegmentCrossesWalkmesh(const WallEdge* walls,
                            int wallCount,
                            const Vector& a,
                            const Vector& b,
                            Vector& outHitPoint,
                            bool ignoreZ = false);

}  // namespace acc::engine
