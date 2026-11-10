# wall_probe.h (57 lines)

Source: `patches/Accessibility/wall_probe.h`

wall probing — "is there a wall, and how far?"

This is the wall system, distinct from the room-shape system that reads it. Everything here is a single-ray or multi-ray cast against the cached perimeter walls (acc::spatial::wall_surfaces::GetCachedWalls); nothing here knows about nav-graph nodes, clusters, or the corridor / junction / Area vocabulary those produce.

The two systems' names got entangled during development: the probes lived in a file called wall_topology.cpp which was, in substance, room topology. The Phase-1 structure pass (refactoring candidate 12) split them by name - the probes became this file, and the rest became room_topology.* on the consumer side.

Probes fail open: with no wall cache, ProbeDistance returns -1 and IsAlcoveAlongAxis returns true, so a caller can never be blocked by missing geometry.

## Declarations (in source order)

- L21 `struct Vector`
- L36 `float ResolveProbeZ(const acc::engine::WallEdge* walls, int wallCount, const Vector& pos)`: Resolve the height a probe from `pos` should cast at.
- L41 `float ProbeDistance(const Vector& pos, float dx, float dy)`: Distance to the first wall along (dx,dy) from `pos`.
- L48 `bool IsAlcoveAlongAxis(const Vector& pos, float forwardX, float forwardY)`: Dead-end shape gate: a 4-ray probe rotated to align with (forwardX,forwardY) - the direction from a degree-1 node toward its graph parent, i.e.
- L53 `void ProbeClearance8(const acc::engine::WallEdge* walls, int wallCount, const Vector& pos, float outRays[8])`: 8-ray clearance probe, filling outRays[8] in octant order (E, NE, N, NW, W, SW, S, SE).
