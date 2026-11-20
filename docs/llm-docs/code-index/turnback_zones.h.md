# turnback_zones.h (50 lines)

Source: `patches/Accessibility/turnback_zones.h`

Trigger areas that the mod's own path stays out of, kept as outlines.

Learned zones: a trigger whose script turned a walk of the mod's back. The rims of Tatooine's deserts are such triggers: a strip along the edge of the map that answers a step into it with a message nobody speaks and puts the party back at the nearest marker. The engine walks to a far target in a straight line where the floor allows it, straight through such a strip. A zone is learned the moment that happens (guidance_approach_rescue.cpp) and forgotten with its area.

Standing zones: a trigger that is known before anyone walks into it, because a step into it kills (deadly_triggers.h). Its script can take such a trigger away, so the set is cleared and given anew whenever it is used.

Engine-free.

## Declarations (in source order)

- L29 `void ForArea(const void* area)`: Forgets the zones of any other area.
- L33 `bool Add(const Vector* poly, int count)`: Adds a learned zone.
- L36 `void ClearStanding()`: Drops the standing zones, and adds one; false as for Add.
- L37 `bool AddStanding(const Vector* poly, int count)`
- L42 `bool Crosses(const Vector& a, const Vector& b)`: True when a walk straight from a to b enters a zone: it crosses the outline of one that holds neither end.
- L43 `bool CrossesStanding(const Vector& a, const Vector& b)`
- L46 `void AppendWalls(const Vector& a, const Vector& b, std::vector<acc::engine::WallEdge>& out)`: Appends the outlines of the zones that hold neither a nor b, as walls.

Also 2 constants or variables at namespace scope.
