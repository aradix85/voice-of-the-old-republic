# audio_footstep_suppress.cpp (637 lines)

Source: `patches/Accessibility/audio_footstep_suppress.cpp`

(keyboard walk keys OR the KOTOR 2 left stick)

## Declarations (in source order)

- L143 `int CollectNearbyBodies(const Vector& pos, Vector* bodies, int cap)`: Nearby blocker bodies (creatures + placeables) within the probe's reach.
- L170 `const char* ProbeBlocker(const Vector& pos, const Vector& b, const acc::engine::WallEdge* walls, int wallCount, const Vector* bodies, int bodyCount)`: What blocks the probe ray pos -> b: "wall", "body", or nullptr when free.
- L197 `void FormatFreeDirections(const bool* freeDir, char* buf, size_t bufSize)`: "Free directions: North, East", or the all-blocked line.
- L221 `void RunStuckProbe(const Vector& pos)`
- L271 `bool IsWalking(uint64_t now_ms)`: Walk-anim freshness: a leader footstep within kFootstepFreshnessMs.
- L279 `void TickCirclingAnnounce(const Vector& pos, uint64_t now_ms, bool walking)`: Fires when walking but oscillating: cumulative path length to net displacement ratio is high over a 4s window.
- L337 `void TickStuckAnnounce(const Vector& pos, uint64_t now_ms, bool walking)`
- L370 `void ResetProbeTriggers()`
- L380 `bool WasStuckLastTick()`
- L382 `bool StuckSustainedFor(unsigned int min_ms)`
- L387 `void NoteLeaderFootstep()`
- L391 `uint64_t LastLeaderFootstepMs()`
- L393 `void Tick()`
- L512 `extern "C" int __cdecl OnPlayFootstep(void* creature)`: CSWCCreature::PlayFootstep detour — BOTH games.
- L618 `extern "C" int __cdecl OnUpdateRollingFootstep(void* creature)`
