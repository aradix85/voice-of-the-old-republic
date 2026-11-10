# audio_stuck_probe.cpp (135 lines)

Source: `patches/Accessibility/audio_stuck_probe.cpp`

The free-directions probe — see audio_stuck_probe.h.

## Declarations (in source order)

- L51 `int CollectNearbyBodies(const Vector& pos, Vector* bodies, int cap)`: Creatures and placeables within the probe's reach; the leader is left out by position.
- L73 `const char* ProbeBlocker(const Vector& pos, const Vector& end, const acc::engine::WallEdge* walls, int wallCount, const Vector* bodies, int bodyCount)`: What blocks the ray pos -> end: "wall", "body", or nullptr when it is free.
- L89 `void RunProbe(const Vector& pos)`
- L116 `void Tick(const Vector& pos, uint64_t now_ms, bool walking)`
- L129 `void Reset()`
