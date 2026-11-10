# engine_radial_diag.cpp (414 lines)

Source: `patches/Accessibility/engine_radial_diag.cpp`

Diagnostic dumps of the target action menu and of a target's door preconditions for the log.

## Declarations (in source order)

- L71 `void ReadResRefLocal(void* base, size_t offset, char* outBuf, size_t bufSize)`: Read a CResRef (fixed 16-byte string) into outBuf, treating the buffer as NUL-terminated within the 16-byte window.
- L96 `void ReadButtonText(void* button, char* outBuf, size_t bufSize)`: Read a CSWGuiButton's text via every path we know about (gui_string -> inline CExoString) into outBuf.
- L106 `void LogState(void* tam, const char* tag)`
- L167 `void LogStateWide(void* tam, const char* tag)`
- L249 `void LogTargetDiag(uint32_t targetClient, const char* tag)`
