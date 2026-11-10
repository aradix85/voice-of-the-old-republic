# engine_area_kinds.cpp (165 lines)

Source: `patches/Accessibility/engine_area_kinds.cpp`

What kind of object this is: a usable placeable, an empty container and its item count, a landmark waypoint, a transition trigger and its geometry, a detected trap, and an object's local booleans.

## Declarations (in source order)

- L16 `bool ReadContainerCount(void* gameObject, int& count)`: Item count of a loot container; false for anything else.
- L40 `bool IsUsablePlaceable(void* placeable)`
- L52 `bool IsEmptyContainer(void* gameObject)`
- L57 `bool IsLandmarkWaypoint(void* waypoint)`
- L67 `bool IsTransitionTrigger(void* trigger)`
- L97 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`
- L116 `bool GetObjectLocalBoolean(void* gameObject, int index)`
- L128 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`
- L159 `int GetContainerItemCount(void* gameObject)`
