# transitions_landmarks.cpp (431 lines)

Source: `patches/Accessibility/transitions_landmarks.cpp`

Landmarks: the per-area cache of named map notes and placeables, the proximity announcement as the player walks past one, and the lookups other modules use (FindLandmarkNear, IterateLandmarks).

## Declarations (in source order)

- L117 `void RecomputeLandmarkRanges()`
- L140 `bool TryAutoDiscoverMapNote(void* waypoint, const Vector& playerPos)`: If `waypoint` (a landmark whose note is still disabled) sits within auto-discover range of the leader, enable it via the engine's own field write.
- L160 `void RebuildLandmarkCache(void* area)`
- L234 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`: Detect map notes whose enabled flag flipped since the last cache scan and rebuild.
- L274 `void TickProximityLandmarks(const Vector& playerPos)`: Per-tick scan over the landmark cache (the proximity note above).
- L360 `bool FindLandmarkNear(const Vector& pos, float rangeM, char* nameOut, size_t nameBufSize, Vector& posOut, int* outLandmarkIdx)`
- L388 `bool IterateLandmarks(void* area, int& cursor, char* nameOut, size_t nameBufSize, Vector& posOut, int& outLandmarkIdx)`
- L417 `void MarkLandmarkClaimedByDoor(int landmarkIdx)`
- L423 `void ResetLandmarkProximityState()`: Landmark proximity firing + the recheck cadence timestamp.
