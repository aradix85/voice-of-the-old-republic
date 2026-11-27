# engine_area_kinds.cpp (177 lines)

Source: `patches/Accessibility/engine_area_kinds.cpp`

What kind of object this is: a usable placeable, an empty container and its item count, a landmark waypoint, a transition trigger and its geometry, a detected trap, and an object's local booleans.

## Declarations (in source order)

- L17 `bool ReadContainerCount(void* gameObject, int& count)`: Item count of a loot container; false for anything else.
- L41 `bool IsUsablePlaceable(void* placeable)`
- L53 `bool IsEmptyContainer(void* gameObject)`
- L58 `bool IsSpentContainer(void* gameObject)`
- L63 `bool IsLandmarkWaypoint(void* waypoint)`
- L67 `bool IsTransitionTrigger(void* trigger)`
- L97 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`
- L116 `int GetTriggerCentroid(void* trigger, Vector& out)`
- L128 `bool GetObjectLocalBoolean(void* gameObject, int index)`
- L140 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`
- L171 `int GetContainerItemCount(void* gameObject)`
