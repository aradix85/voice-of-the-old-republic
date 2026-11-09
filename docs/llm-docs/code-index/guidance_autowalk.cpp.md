# guidance_autowalk.cpp (374 lines)

Source: `patches/Accessibility/guidance_autowalk.cpp`

movement-key cancel.

## Declarations (in source order)

- L101 `void PrimeActionManager(void* creature, int mode)`
- L112 `int ReadServerObjInt(void* obj, size_t off, int dflt)`: Read an int field off a server object with SEH protection.
- L124 `bool WalkTo(const Vector& destination)`
- L214 `bool ForceWalkTo(const Vector& destination)`
- L260 `bool UseObject(unsigned long targetHandle)`
- L300 `bool CancelMovement()`
- L325 `void PollMovementKeysCancel()`
