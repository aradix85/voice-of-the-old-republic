# guidance_autowalk.cpp (384 lines)

Source: `patches/Accessibility/guidance_autowalk.cpp`

movement-key cancel.

## Declarations (in source order)

- L102 `void PrimeActionManager(void* creature, int mode)`
- L113 `int ReadServerObjInt(void* obj, size_t off, int dflt)`: Read an int field off a server object with SEH protection.
- L132 `Vector WalkEnd(const Vector& destination)`: Where a walk to `destination` ends.
- L146 `bool WalkTo(const Vector& destination)`
- L236 `bool ForceWalkTo(const Vector& destination)`
- L282 `bool UseObject(unsigned long targetHandle)`
- L313 `bool CancelMovement()`
- L338 `void PollMovementKeysCancel()`
