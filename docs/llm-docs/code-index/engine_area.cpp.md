# engine_area.cpp (1448 lines)

Source: `patches/Accessibility/engine_area.cpp`

(From engine_area.h.) Per-area object iteration + room lookup. SEH-guarded; no engine re-entry beyond CSWSArea::GetRoom.

CSWSArea: +0x190 game_objects      ulong*  — HANDLE array (not CSWSObject**). +0x194 game_object_count int +0x230 rooms             CSWSRoom* — POINTER to an inline-stride (0x4c) array; deref before indexing. +0x25c room_names        CExoString*

CSWSArea::GetRoom @0x4bb600(this, Vector*, int* outRoomIndex /*nullable) → CSWSRoom*

CSWSObject base: +0x008 object_kind  uint8 GAME_OBJECT_TYPES +0x090 position     Vector

Handle resolution chain: *kAddrAppManagerPtr → AppManager → +0x4 CClientExoApp* / +0x8 CServerExoApp*. The server path is used for object resolution (CSWSObject::GetArea @0x4cb120 confirms +0x8 via `mov ecx,[eax+0x8]`). CServerExoApp::GetObjectArray @0x004aed70 → CGameObjectArray*. CGameObjectArray::GetGameObject(id, &outPtr) @0x004d8230 → bool.

game_objects[] is an array of handles, not pointers. Initial code treated entries as CSWSObject** and got kind-byte garbage; iteration now resolves each handle through GetGameObject.

## Declarations (in source order)

- L50 `void* GetServerObjectArray()`: CServerExoApp → GetObjectArray() → CGameObjectArray*.
- L64 `void* GetCurrentArea()`
- L68 `int GetObjectKind(void* gameObject)`
- L78 `uint32_t GetObjectHandle(void* gameObject)`
- L97 `bool IsSentinelHandle(uint32_t handle)`: Engine sentinels shared by both resolve paths: 0 (uninitialised), 0xFFFFFFFF (removed), 0x7F000000 (kInvalidObjectId — the "no object" marker the action queue and LastTarget use).
- L103 `void* ResolveServerObjectHandle(uint32_t handle)`
- L134 `void* ResolveClientObject(uint32_t handle)`
- L149 `void* ResolveClientObjectHandle(uint32_t handle)`
- L164 `bool GetObjectPosition(void* gameObject, Vector& out)`
- L176 `void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex)`
- L197 `bool GetRoomRepresentativeWorld(void* area, int roomIdx, Vector& outWorld, int* outFailReason)`
- L271 `bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize)`
- L290 `bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize)`
- L300 `bool GetAreaTag(void* area, char* outBuf, size_t bufSize)`
- L310 `bool GetRoomDisplayName(void* area, int roomIndex, char* outBuf, size_t bufSize)`
- L330 `AreaObjectIterator::AreaObjectIterator(void* area) : handles_(nullptr), size_(0), index_(0), objectArray_(nullptr)`
- L355 `void SubstitutePlayerNameToken(char* buf, size_t bufSize)`: → the PC's name.
- L393 `bool TryReadLocString(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`: Resolve a CExoLocString by treating its 8 bytes as the same shape as CExoString (char* + uint32 length); fall back to TLK strref at +4.
- L401 `bool TryReadTag(void* obj, char* outBuf, size_t bufSize)`: Last-resort: speak the modder-assigned tag instead of an empty name.
- L426 `void CleanAuthoredName(char* buf, size_t bufSize)`: Strip the two kinds of authoring residue KOTOR 2's object names carry into speech.
- L446 `void AppendCommaSeparated(char* outBuf, size_t bufSize, const char* text)`: Append ", " to outBuf if there's room.
- L465 `bool ReadDoorFlags(void* serverDoor, uint32_t& locked, uint8_t& openState)`: One SEH-guarded read of the two live door state fields.
- L488 `void BuildDoorLockSnapshot(void* area)`
- L519 `void AppendLockNeed(void* obj, char* outBuf, size_t bufSize)`: ", opens only through the story" / ", needs a key" / ", Security 15" after the "locked" word, so the player knows whether to bring a key, a slicer, or nothing at all (KOTOR 1; see ReadLockNeed).
- L564 `void BuildDoorSuffix(void* serverDoor, char* outBuf, size_t bufSize)`: Build the comma-prefixed suffix for a CSWSDoor — state + transition destination + description.
- L633 `static bool TryResolveDisplayNameOnce(void* clientApp, uint32_t handle, char* outBuf, size_t bufSize)`: Inner: one engine call with the exact handle we got.
- L658 `bool GetObjectDisplayNameByHandle(uint32_t handle, char* outBuf, size_t bufSize)`
- L691 `bool WasDoorLockedAtAreaLoad(void* serverDoor)`
- L716 `void ResetDoorLockSnapshot()`
- L721 `void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L742 `bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize)`
- L748 `bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize)`
- L874 `bool IsUsablePlaceable(void* placeable)`
- L886 `bool IsEmptyContainer(void* gameObject)`
- L916 `bool IsLandmarkWaypoint(void* waypoint)`
- L926 `bool IsTransitionTrigger(void* trigger)`
- L964 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`
- L983 `bool GetObjectLocalBoolean(void* gameObject, int index)`
- L995 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`
- L1026 `bool IsDoorOpen(void* serverDoor)`
- L1036 `bool IsDoorLocked(void* serverDoor)`
- L1046 `bool DoorHasTransitionDest(void* serverDoor)`
- L1067 `bool ReadLockFieldsRaw(void* obj, size_t lockedOff, size_t keyReqOff, size_t dcOff, bool dcIsShort, LockFields& out)`: One SEH-guarded read of an object's lock fields.
- L1083 `void LogLockOnce(void* obj, int kind, const LockFields& f, const char* key, LockNeed need, const char* failScript = "")`: One log line per object (recent handles remembered), so the raw values can be checked against the object's UTD / UTP template.
- L1104 `LockNeed ReadLockNeed(void* gameObject, int& outDc, bool* outLocked)`
- L1172 `int GetContainerItemCount(void* gameObject)`
- L1193 `bool IsDoorStatic(void* serverDoor)`
- L1204 `bool IsDoorCosmetic(void* serverDoor)`
- L1238 `bool MaybeDrivePassiveSelection()`: Root-cause fix (2026-07-19, rev 2): the Endar Spire opening HOLDS the global fade panel at alpha=1.0 for a long scripted stretch *after* the fade animation finishes (IsFadeDone), while the player still has full world ...
- L1401 `DoorMaterial GetDoorMaterial(void* serverDoor)`
- L1416 `void* AreaObjectIterator::Next()`
