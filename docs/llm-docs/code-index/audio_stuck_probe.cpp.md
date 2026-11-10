# audio_stuck_probe.cpp (322 lines)

Source: `patches/Accessibility/audio_stuck_probe.cpp`

The stuck-direction probe: when the leader walks without getting anywhere, or circles, speak once which of the eight directions are free.

## Declarations (in source order)

- L90 `int CollectNearbyBodies(const Vector& pos, Vector* bodies, int cap)`: Nearby blocker bodies (creatures + placeables) within the probe's reach.
- L117 `const char* ProbeBlocker(const Vector& pos, const Vector& b, const acc::engine::WallEdge* walls, int wallCount, const Vector* bodies, int bodyCount)`: What blocks the probe ray pos -> b: "wall", "body", or nullptr when free.
- L144 `void FormatFreeDirections(const bool* freeDir, char* buf, size_t bufSize)`: "Free directions: North, East", or the all-blocked line.
- L168 `void RunStuckProbe(const Vector& pos)`
- L222 `void TickCirclingAnnounce(const Vector& pos, uint64_t now_ms, bool walking)`: Fires when walking but oscillating: cumulative path length to net displacement ratio is high over a 4s window.
- L280 `void TickStuckAnnounce(const Vector& pos, uint64_t now_ms, bool walking)`
- L313 `void ResetProbeTriggers()`
