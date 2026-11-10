# transitions_internal.h (52 lines)

Source: `patches/Accessibility/transitions_internal.h`

Internal to the transitions module (transitions*.cpp); no other file includes it. The public interface is transitions.h. Here: what the files share.

## Declarations (in source order)

- L32 `void TrackMotion(const Vector& pos)`: transitions_centre.cpp
- L33 `void ResetMotion()`
- L34 `void ArmCentreRule(int clusterId, const acc::room_topology::ClusterInfo& info, const Vector& pos, DWORD sinceMs)`
- L36 `bool CentreRuleReady(int clusterId, const Vector& pos)`
- L39 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`: transitions_room_speech.cpp
- L40 `void NoteGatedClusterChange()`
- L41 `void TickPendingPlaza(void* area, const Vector& playerPos)`
- L42 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`
- L43 `void ResetRoomSpeechState()`
- L46 `void RebuildLandmarkCache(void* area)`: transitions_landmarks.cpp
- L47 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`
- L48 `void TickProximityLandmarks(const Vector& playerPos)`
- L49 `void ResetLandmarkProximityState()`

Also 2 constants or variables at namespace scope.
