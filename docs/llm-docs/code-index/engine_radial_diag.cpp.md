# engine_radial_diag.cpp (385 lines)

Source: `patches/Accessibility/engine_radial_diag.cpp`

Diagnostic dumps of the target action menu and of a target's door preconditions for the log.

## Declarations (in source order)

- L55 `void ReadResRef(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L78 `void LogState(void* tam, const char* tag)`
- L139 `void LogStateWide(void* tam, const char* tag)`
- L221 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
