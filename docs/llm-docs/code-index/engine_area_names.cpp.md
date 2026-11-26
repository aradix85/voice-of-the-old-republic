# engine_area_names.cpp (283 lines)

Source: `patches/Accessibility/engine_area_names.cpp`

Object names as the mod speaks them: the authored name cleaned up, player-name tokens filled in, a fallback for unnamed objects, and the state suffix (door, lock, container) appended.

## Declarations (in source order)

- L20 `void SubstitutePlayerNameToken(char* buf, size_t bufSize)`: → the PC's name, in place.
- L49 `void CleanAuthoredText(char* buf, size_t bufSize)`
- L59 `bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Resolve a CExoLocString by treating its 8 bytes as the same shape as CExoString (char* + uint32 length); fall back to TLK strref at +4.
- L67 `void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text)`: Append ", " to outBuf if there's room.
- L86 `static bool TryResolveDisplayNameOnce(void* clientApp, uint32_t handle, char* outBuf, size_t bufSize)`: Inner: one engine call with the exact handle we got.
- L107 `bool GetObjectDisplayNameByHandle(uint32_t handle, char* outBuf, size_t bufSize)`
- L132 `void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L153 `bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize)`
- L159 `bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize)`
