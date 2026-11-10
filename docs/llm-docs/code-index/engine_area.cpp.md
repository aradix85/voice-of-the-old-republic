# engine_area.cpp (261 lines)

Source: `patches/Accessibility/engine_area.cpp`

Game objects and areas: the server object array, handle resolution (server and client side), object kind, position and tag, area and room names, and the iterator over an area's objects.

## Declarations (in source order)

- L48 `void* GetServerObjectArray()`: CServerExoApp → GetObjectArray() → CGameObjectArray*.
- L63 `void* LookupGameObject(void* objectArray, uint32_t id)`: CGameObjectArray::GetGameObject returns *false on hit, true on miss*: the "no match" branch falls through to `return true` after writing NULL to the out-param (decompiled @0x004d8230).
- L78 `void* GetCurrentArea()`
- L82 `int GetObjectKind(void* gameObject)`
- L92 `uint32_t GetObjectHandle(void* gameObject)`
- L109 `bool IsSentinelHandle(uint32_t handle)`: Engine sentinels shared by both resolve paths: 0 (uninitialised), 0xFFFFFFFF (removed), 0x7F000000 (kInvalidObjectId — the "no object" marker the action queue and LastTarget use).
- L113 `void* ResolveServerObjectHandle(uint32_t handle)`
- L120 `void* ResolveClientObject(uint32_t handle)`
- L135 `void* ResolveClientObjectHandle(uint32_t handle)`
- L150 `bool GetObjectPosition(void* gameObject, Vector& out)`
- L162 `void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex)`
- L183 `bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize)`
- L197 `bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize)`
- L203 `bool GetAreaTag(void* area, char* outBuf, size_t bufSize)`
- L209 `bool GetRoomDisplayName(void* area, int roomIndex, char* outBuf, size_t bufSize)`
- L229 `AreaObjectIterator::AreaObjectIterator(void* area) : handles_(nullptr), size_(0), index_(0), objectArray_(nullptr)`
- L244 `void* AreaObjectIterator::Next()`
