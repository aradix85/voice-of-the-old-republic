# engine_radial_diag.cpp (389 lines)

Source: `patches/Accessibility/engine_radial_diag.cpp`

Diagnostic dumps of the target action menu and of a target's door preconditions for the log.

## Declarations (in source order)

- L59 `void ReadResRef(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L82 `void LogState(void* tam, const char* tag)`
- L143 `void LogStateWide(void* tam, const char* tag)`
- L225 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
