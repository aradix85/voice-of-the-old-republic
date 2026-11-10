# engine_area.cpp (289 lines)

Source: `patches/Accessibility/engine_area.cpp`

Game objects and areas: the server object array, handle resolution (server and client side), object kind, position and tag, area and room names, and the iterator over an area's objects.

## Declarations (in source order)

- L46 `void* GetServerObjectArray()`: CServerExoApp → GetObjectArray() → CGameObjectArray*.
- L60 `void* GetCurrentArea()`
- L64 `int GetObjectKind(void* gameObject)`
- L74 `uint32_t GetObjectHandle(void* gameObject)`
- L91 `bool IsSentinelHandle(uint32_t handle)`: Engine sentinels shared by both resolve paths: 0 (uninitialised), 0xFFFFFFFF (removed), 0x7F000000 (kInvalidObjectId — the "no object" marker the action queue and LastTarget use).
- L95 `void* ResolveServerObjectHandle(uint32_t handle)`
- L116 `void* ResolveClientObject(uint32_t handle)`
- L131 `void* ResolveClientObjectHandle(uint32_t handle)`
- L146 `bool GetObjectPosition(void* gameObject, Vector& out)`
- L158 `void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex)`
- L179 `bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize)`
- L198 `bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize)`
- L208 `bool GetAreaTag(void* area, char* outBuf, size_t bufSize)`
- L218 `bool GetRoomDisplayName(void* area, int roomIndex, char* outBuf, size_t bufSize)`
- L238 `AreaObjectIterator::AreaObjectIterator(void* area) : handles_(nullptr), size_(0), index_(0), objectArray_(nullptr)`
- L257 `void* AreaObjectIterator::Next()`
