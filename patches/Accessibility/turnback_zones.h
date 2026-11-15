// Trigger areas whose script turned a walk of the mod's back, kept as
// outlines that the mod's own path stays out of.
//
// The rims of Tatooine's deserts are such triggers: a strip along the edge of
// the map that answers a step into it with a message nobody speaks and puts
// the party back at the nearest marker. The engine walks to a far target in a
// straight line where the floor allows it, straight through such a strip. A
// zone is learned the moment that happens (guidance_approach_rescue.cpp) and
// forgotten with its area. Engine-free.

#pragma once

#include <vector>

#include "walkmesh_geometry.h"  // WallEdge

namespace acc::guidance::turnback {

constexpr int kMaxZones = 16;
constexpr int kMaxVerts = 32;  // the most GetTriggerGeometry hands out

// Forgets the zones of any other area. Call before the functions below.
void ForArea(const void* area);

// Adds a zone. False when it is known already, has fewer than three or more
// than kMaxVerts corners, or the store is full.
bool Add(const Vector* poly, int count);

// True when a walk straight from a to b enters a zone: it crosses the outline
// of one that holds neither end. A walk out of a zone, or to a target inside
// one, is left alone.
bool Crosses(const Vector& a, const Vector& b);

// Appends the outlines of the zones that hold neither a nor b, as walls.
void AppendWalls(const Vector& a, const Vector& b,
                 std::vector<acc::engine::WallEdge>& out);

}  // namespace acc::guidance::turnback
