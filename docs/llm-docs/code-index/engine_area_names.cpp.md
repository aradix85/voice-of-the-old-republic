# engine_area_names.cpp (326 lines)

Source: `patches/Accessibility/engine_area_names.cpp`

Object names as the mod speaks them: the authored name cleaned up, player-name tokens filled in, a fallback for unnamed objects, and the state suffix (door, lock, container) appended.

## Declarations (in source order)

- L37 `void SubstitutePlayerNameToken(char* buf, size_t bufSize)`: → the PC's name, in place: KOTOR 1's one token, by hand.
- L66 `bool ExpandTextTokens(char* buf, size_t bufSize)`
- L86 `void CleanAuthoredText(char* buf, size_t bufSize)`
- L102 `bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Resolve a CExoLocString by treating its 8 bytes as the same shape as CExoString (char* + uint32 length); fall back to TLK strref at +4.
- L110 `void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text)`: Append ", " to outBuf if there's room.
- L129 `static bool TryResolveDisplayNameOnce(void* clientApp, uint32_t handle, char* outBuf, size_t bufSize)`: Inner: one engine call with the exact handle we got.
- L150 `bool GetObjectDisplayNameByHandle(uint32_t handle, char* outBuf, size_t bufSize)`
- L175 `void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L196 `bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize)`
- L202 `bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize)`
