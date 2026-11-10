# engine_area_names.cpp (342 lines)

Source: `patches/Accessibility/engine_area_names.cpp`

Object names as the mod speaks them: the authored name cleaned up, player-name tokens filled in, a fallback for unnamed objects, and the state suffix (door, lock, container) appended.

## Declarations (in source order)

- L31 `void CleanAuthoredName(char* buf, size_t bufSize)`: Strip the two kinds of authoring residue KOTOR 2's object names carry into speech.
- L57 `void SubstitutePlayerNameToken(char* buf, size_t bufSize)`: → the PC's name.
- L93 `bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Resolve a CExoLocString by treating its 8 bytes as the same shape as CExoString (char* + uint32 length); fall back to TLK strref at +4.
- L101 `bool TryReadTag(void* obj, char* outBuf, size_t bufSize)`: Last-resort: speak the modder-assigned tag instead of an empty name.
- L111 `void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text)`: Append ", " to outBuf if there's room.
- L130 `static bool TryResolveDisplayNameOnce(void* clientApp, uint32_t handle, char* outBuf, size_t bufSize)`: Inner: one engine call with the exact handle we got.
- L155 `bool GetObjectDisplayNameByHandle(uint32_t handle, char* outBuf, size_t bufSize)`
- L188 `void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L209 `bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize)`
- L215 `bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize)`
