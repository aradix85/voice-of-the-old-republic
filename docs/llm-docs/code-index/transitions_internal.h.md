# transitions_internal.h (233 lines)

Source: `patches/Accessibility/transitions_internal.h`

Internal to the transitions module (transitions*.cpp); no other file includes it. The public interface is transitions.h. Here: what the files share.

## Declarations (in source order)

- L19 `void EnsureSpeechInitialized()`: Forward decl from core_dllmain.cpp.
- L210 `bool IsWorldSpeechGatedImpl()`: transitions.cpp
- L213 `void TrackMotion(const Vector& pos)`: transitions_centre.cpp
- L214 `void ResetMotion()`
- L215 `float CentreDistance(const Vector& pos)`
- L216 `bool CentreRuleReady(int clusterId, const Vector& pos)`
- L219 `void SpeakRoomChange(void* area, int clusterId, const Vector& worldPos)`: transitions_room_speech.cpp
- L220 `void TickPendingPlatz(void* area, const Vector& playerPos)`
- L221 `void TickGatedClusterRefire(void* area, const Vector& playerPos)`
- L222 `void ResetRoomSpeechState()`
- L223 `void ResetPendingPlatzState()`
- L224 `void ResetGatedRefireState()`
- L227 `void RebuildLandmarkCache(void* area)`: transitions_landmarks.cpp
- L228 `void TickLandmarkCacheRecheck(void* area, const Vector& playerPos)`
- L229 `void TickProximityLandmarks(const Vector& playerPos)`
- L230 `void ResetLandmarkProximityState()`

Also 39 constants or variables at namespace scope.
