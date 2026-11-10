# engine_area_doors.cpp (228 lines)

Source: `patches/Accessibility/engine_area_doors.cpp`

Door state: open, locked, static, cosmetic, a transition destination, the doors that were locked when the area loaded, and the door part of a spoken name.

## Declarations (in source order)

- L27 `bool ReadDoorFlags(void* serverDoor, uint32_t& locked, uint8_t& openState)`: One SEH-guarded read of the two live door state fields.
- L39 `void BuildDoorLockSnapshot(void* area)`
- L78 `void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize)`: Build the comma-prefixed suffix for a CSWSDoor — state + transition destination + description.
- L142 `bool WasDoorLockedAtAreaLoad(void* serverDoor)`
- L167 `void ResetDoorLockSnapshot()`
- L172 `bool IsDoorOpen(void* serverDoor)`
- L182 `bool IsDoorLocked(void* serverDoor)`
- L192 `bool DoorHasTransitionDest(void* serverDoor)`
- L203 `bool IsDoorStatic(void* serverDoor)`
- L214 `bool IsDoorCosmetic(void* serverDoor)`
