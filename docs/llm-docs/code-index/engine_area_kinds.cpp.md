# engine_area_kinds.cpp (181 lines)

Source: `patches/Accessibility/engine_area_kinds.cpp`

What kind of object this is: a usable placeable, an empty container and its item count, a landmark waypoint, a transition trigger and its geometry, a detected trap, and an object's local booleans.

## Declarations (in source order)

- L7 `bool IsUsablePlaceable(void* placeable)`
- L19 `bool IsEmptyContainer(void* gameObject)`
- L49 `bool IsLandmarkWaypoint(void* waypoint)`
- L59 `bool IsTransitionTrigger(void* trigger)`
- L97 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`
- L116 `bool GetObjectLocalBoolean(void* gameObject, int index)`
- L128 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`
- L159 `int GetContainerItemCount(void* gameObject)`
