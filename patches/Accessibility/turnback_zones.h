// Trigger areas that the mod's own path stays out of, kept as outlines.
//
// Learned zones: a trigger whose script turned a walk of the mod's back. The
// rims of Tatooine's deserts are such triggers: a strip along the edge of
// the map that answers a step into it with a message nobody speaks and puts
// the party back at the nearest marker. The engine walks to a far target in a
// straight line where the floor allows it, straight through such a strip. A
// zone is learned the moment that happens (guidance_approach_rescue.cpp) and
// forgotten with its area.
//
// Standing zones: a trigger that is known before anyone walks into it, because
// a step into it kills (deadly_triggers.h). Its script can take such a trigger
// away, so the set is cleared and given anew whenever it is used.
//
// Engine-free.

#pragma once

#include <vector>

#include "walkmesh_geometry.h"  // WallEdge

namespace acc::guidance::turnback {

constexpr int kMaxZones = 32;
constexpr int kMaxVerts = 32;  // the most GetTriggerGeometry hands out

// Forgets the zones of any other area. Call before the functions below.
void ForArea(const void* area);

// Adds a learned zone. False when it is known already, has fewer than three
// or more than kMaxVerts corners, or the store is full.
bool Add(const Vector* poly, int count);

// Drops the standing zones, and adds one; false as for Add.
void ClearStanding();
bool AddStanding(const Vector* poly, int count);

// True when a walk straight from a to b enters a zone: it crosses the outline
// of one that holds neither end. A walk out of a zone, or to a target inside
// one, is left alone. CrossesStanding looks at the standing zones only.
bool Crosses(const Vector& a, const Vector& b);
bool CrossesStanding(const Vector& a, const Vector& b);

// Appends the outlines of the zones that hold neither a nor b, as walls.
void AppendWalls(const Vector& a, const Vector& b,
                 std::vector<acc::engine::WallEdge>& out);

}  // namespace acc::guidance::turnback
