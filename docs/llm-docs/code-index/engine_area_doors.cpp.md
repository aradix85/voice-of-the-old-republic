# engine_area_doors.cpp (222 lines)

Source: `patches/Accessibility/engine_area_doors.cpp`

Door state: open, locked, static, cosmetic, a transition destination, the doors that were locked when the area loaded, and the door part of a spoken name.

## Declarations (in source order)

- L28 `bool ReadDoorFlags(void* serverDoor, uint32_t& locked, uint8_t& openState)`: One SEH-guarded read of the two live door state fields.
- L40 `void BuildDoorLockSnapshot(void* area)`
- L79 `void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize)`: Build the comma-prefixed suffix for a CSWSDoor — state + transition destination + description.
- L140 `bool WasDoorLockedAtAreaLoad(void* serverDoor)`
- L165 `void ResetDoorLockSnapshot()`
- L170 `bool IsDoorOpen(void* serverDoor)`
- L176 `bool IsDoorLocked(void* serverDoor)`
- L186 `bool DoorHasTransitionDest(void* serverDoor)`
- L197 `bool IsDoorStatic(void* serverDoor)`
- L208 `bool IsDoorCosmetic(void* serverDoor)`
