# transitions_landmarks.cpp (433 lines)

Source: `patches/Accessibility/transitions_landmarks.cpp`

Landmarks: the per-area cache of named map notes and placeables, the proximity announcement as the player walks past one, and the lookups other modules use (FindLandmarkNear, IterateLandmarks).

## Declarations (in source order)

- L105 `void RecomputeLandmarkRanges()`
- L128 `bool TryAutoDiscoverMapNote(void* waypoint, const Vector& playerPos)`: If `waypoint` (a landmark whose note is still disabled) sits within auto-discover range of the leader, enable it via the engine's own field write.
- L148 `void RebuildLandmarkCache(void* area)`
- L230 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`: Detect map notes whose enabled flag flipped since the last cache scan and rebuild.
- L274 `void TickProximityLandmarks(const Vector& playerPos)`: Per-tick scan over the landmark cache.
- L361 `bool FindLandmarkNear(const Vector& pos, float rangeM, char* nameOut, size_t nameBufSize, Vector& posOut, int* outLandmarkIdx)`
- L389 `bool IterateLandmarks(void* area, int& cursor, char* nameOut, size_t nameBufSize, Vector& posOut, int& outLandmarkIdx)`
- L419 `void MarkLandmarkClaimedByDoor(int landmarkIdx)`
- L425 `void ResetLandmarkProximityState()`: Landmark proximity firing + the recheck cadence timestamp.
