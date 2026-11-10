# guidance_autowalk.cpp (360 lines)

Source: `patches/Accessibility/guidance_autowalk.cpp`

movement-key cancel.

## Declarations (in source order)

- L100 `void PrimeActionManager(void* creature, int mode)`
- L111 `int ReadServerObjInt(void* obj, size_t off, int dflt)`: Read an int field off a server object with SEH protection.
- L123 `bool WalkTo(const Vector& destination)`
- L213 `bool ForceWalkTo(const Vector& destination)`
- L259 `bool UseObject(unsigned long targetHandle)`
- L290 `bool CancelMovement()`
- L315 `void PollMovementKeysCancel()`
