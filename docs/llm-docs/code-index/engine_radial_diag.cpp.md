# engine_radial_diag.cpp (369 lines)

Source: `patches/Accessibility/engine_radial_diag.cpp`

Diagnostic dumps of the target action menu and of a target's door preconditions for the log.

## Declarations (in source order)

- L56 `void ReadResRef(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L79 `void LogState(void* tam, const char* tag)`
- L140 `void LogStateWide(void* tam, const char* tag)`
- L222 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
