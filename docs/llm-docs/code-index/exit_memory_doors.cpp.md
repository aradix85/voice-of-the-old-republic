# exit_memory_doors.cpp (49 lines)

Source: `patches/Accessibility/exit_memory_doors.cpp`

Door lookups near a point for the exit-memory modules: locked doors, the closed or any door at a spot, and the nearest door into another area.

## Declarations (in source order)

- L11 `std::vector<Vector> LockedDoorPositions(void* area)`
- L22 `void* ClosedDoorNear(void* area, const Vector& p)`: The closed door within kDoorMatchM of `p`, or null.
- L29 `void* DoorNear(void* area, const Vector& p)`: The door (open or closed) within kDoorMatchM of `p`, or null.
- L35 `void* TransitionDoorNear(void* area, const Vector& p, float maxM, const std::string& destName, Vector* outPos)`: The nearest door into another area within `maxM` of `p`, or null.
