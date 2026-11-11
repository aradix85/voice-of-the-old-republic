# wall_probe.cpp (259 lines)

Source: `patches/Accessibility/wall_probe.cpp`

wall probing — single- and multi-ray casts against the cached walls.

Split out of room_topology.cpp by the Phase-1 structure pass (refactoring candidate 12). See wall_probe.h for why this is its own system: the file it came from is really room topology, and the two had grown entangled under one name.

Self-contained by construction - the probes read the perimeter-wall cache directly and hold no state of their own. ProbeWall stays private here; the three published entry points are the only ones room topology (or anything else) needs.

## Declarations (in source order)

- L52 `float ResolveProbeZ(const acc::engine::WallEdge* walls, int wallCount, const Vector& pos)`: Probe-height vote (see wall_probe.h).
- L151 `float ProbeWall(const acc::engine::WallEdge* walls, int wallCount, const Vector& origin, float dx, float dy)`
- L169 `float ProbeDistance(const Vector& pos, float dx, float dy)`: Distance to first wall along (dx,dy).
- L202 `bool IsAlcoveAlongAxis(const Vector& pos, float forwardX, float forwardY)`: Dead-end shape gate, a 4-ray probe rotated to align with (forwardX,forwardY) — the direction from the degree-1 node toward its graph parent, i.e.
- L245 `void ProbeClearance8(const acc::engine::WallEdge* walls, int wallCount, const Vector& pos, float outRays[8])`: 8-ray clearance probe.
