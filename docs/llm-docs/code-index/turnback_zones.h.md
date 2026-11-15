# turnback_zones.h (39 lines)

Source: `patches/Accessibility/turnback_zones.h`

Trigger areas whose script turned a walk of the mod's back, kept as outlines that the mod's own path stays out of.

The rims of Tatooine's deserts are such triggers: a strip along the edge of the map that answers a step into it with a message nobody speaks and puts the party back at the nearest marker. The engine walks to a far target in a straight line where the floor allows it, straight through such a strip. A zone is learned the moment that happens (guidance_approach_rescue.cpp) and forgotten with its area. Engine-free.

## Declarations (in source order)

- L23 `void ForArea(const void* area)`: Forgets the zones of any other area.
- L27 `bool Add(const Vector* poly, int count)`: Adds a zone.
- L32 `bool Crosses(const Vector& a, const Vector& b)`: True when a walk straight from a to b enters a zone: it crosses the outline of one that holds neither end.
- L35 `void AppendWalls(const Vector& a, const Vector& b, std::vector<acc::engine::WallEdge>& out)`: Appends the outlines of the zones that hold neither a nor b, as walls.

Also 2 constants or variables at namespace scope.
