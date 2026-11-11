# engine_area.cpp (242 lines)

Source: `patches/Accessibility/engine_area.cpp`

Game objects and areas: the server object array, handle resolution (server and client side), object kind, position and tag, area and room names, and the iterator over an area's objects.

## Declarations (in source order)

- L48 `void* GetServerObjectArray()`: CServerExoApp → GetObjectArray() → CGameObjectArray*.
- L63 `void* LookupGameObject(void* objectArray, uint32_t id)`: CGameObjectArray::GetGameObject returns *false on hit, true on miss*: the "no match" branch falls through to `return true` after writing NULL to the out-param (decompiled @0x004d8230).
- L78 `void* GetCurrentArea()`
- L82 `int GetObjectKind(void* gameObject)`
- L87 `uint32_t GetObjectHandle(void* gameObject)`
- L104 `bool IsSentinelHandle(uint32_t handle)`: Engine sentinels shared by both resolve paths: 0 (uninitialised), 0xFFFFFFFF (removed), 0x7F000000 (kInvalidObjectId — the "no object" marker the action queue and LastTarget use).
- L108 `void* ResolveServerObjectHandle(uint32_t handle)`
- L115 `void* ResolveClientObject(uint32_t handle)`
- L130 `void* ResolveClientObjectHandle(uint32_t handle)`
- L139 `bool GetObjectPosition(void* gameObject, Vector& out)`
- L143 `void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex)`
- L164 `bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize)`
- L178 `bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize)`
- L184 `bool GetAreaTag(void* area, char* outBuf, size_t bufSize)`
- L190 `bool GetRoomDisplayName(void* area, int roomIndex, char* outBuf, size_t bufSize)`
- L210 `AreaObjectIterator::AreaObjectIterator(void* area) : handles_(nullptr), size_(0), index_(0), objectArray_(nullptr)`
- L225 `void* AreaObjectIterator::Next()`
