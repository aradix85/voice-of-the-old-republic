# guidance_autowalk.cpp (357 lines)

Source: `patches/Accessibility/guidance_autowalk.cpp`

movement-key cancel.

## Declarations (in source order)

- L101 `void PrimeActionManager(void* creature, int mode)`
- L112 `int ReadServerObjInt(void* obj, size_t off, int dflt)`: Read an int field off a server object with SEH protection.
- L120 `bool WalkTo(const Vector& destination)`
- L210 `bool ForceWalkTo(const Vector& destination)`
- L256 `bool UseObject(unsigned long targetHandle)`
- L287 `bool CancelMovement()`
- L312 `void PollMovementKeysCancel()`
