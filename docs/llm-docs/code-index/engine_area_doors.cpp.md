# engine_area_doors.cpp (216 lines)

Source: `patches/Accessibility/engine_area_doors.cpp`

Door state: open, locked, static, cosmetic, a transition destination, the doors that were locked when the area loaded, and the door part of a spoken name.

## Declarations (in source order)

- L29 `bool ReadDoorFlags(void* serverDoor, uint32_t& locked, uint8_t& openState)`: One SEH-guarded read of the two live door state fields.
- L41 `void BuildDoorLockSnapshot(void* area)`
- L80 `void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize)`: Build the comma-prefixed suffix for a CSWSDoor — state + transition destination + description.
- L141 `bool WasDoorLockedAtAreaLoad(void* serverDoor)`
- L166 `void ResetDoorLockSnapshot()`
- L171 `bool IsDoorOpen(void* serverDoor)`
- L177 `bool IsDoorLocked(void* serverDoor)`
- L187 `bool DoorHasTransitionDest(void* serverDoor)`
- L198 `bool IsDoorStatic(void* serverDoor)`
- L202 `bool IsDoorCosmetic(void* serverDoor)`
