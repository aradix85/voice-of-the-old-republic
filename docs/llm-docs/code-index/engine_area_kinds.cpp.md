# engine_area_kinds.cpp (160 lines)

Source: `patches/Accessibility/engine_area_kinds.cpp`

What kind of object this is: a usable placeable, an empty container and its item count, a landmark waypoint, a transition trigger and its geometry, a detected trap, and an object's local booleans.

## Declarations (in source order)

- L17 `bool ReadContainerCount(void* gameObject, int& count)`: Item count of a loot container; false for anything else.
- L41 `bool IsUsablePlaceable(void* placeable)`
- L53 `bool IsEmptyContainer(void* gameObject)`
- L58 `bool IsLandmarkWaypoint(void* waypoint)`
- L62 `bool IsTransitionTrigger(void* trigger)`
- L92 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`
- L111 `bool GetObjectLocalBoolean(void* gameObject, int index)`
- L123 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`
- L154 `int GetContainerItemCount(void* gameObject)`
