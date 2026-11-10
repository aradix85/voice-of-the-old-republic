# engine_area.h (800 lines)

Source: `patches/Accessibility/engine_area.h`

Per-area object iteration + room lookup. SEH-guarded; no engine re-entry beyond CSWSArea::GetRoom.

CSWSArea: +0x190 game_objects      ulong*  — HANDLE array (not CSWSObject**). +0x194 game_object_count int +0x230 rooms             CSWSRoom* — POINTER to an inline-stride (0x4c) array; deref before indexing. +0x25c room_names        CExoString*

CSWSArea::GetRoom @0x4bb600(this, Vector*, int* outRoomIndex /*nullable) → CSWSRoom*

CSWSObject base: +0x008 object_kind  uint8 GAME_OBJECT_TYPES +0x090 position     Vector

Handle resolution chain: *kAddrAppManagerPtr → AppManager → +0x4 CClientExoApp* / +0x8 CServerExoApp*. The server path is used for object resolution (CSWSObject::GetArea @0x4cb120 confirms +0x8 via `mov ecx,[eax+0x8]`). CServerExoApp::GetObjectArray @0x004aed70 → CGameObjectArray*. CGameObjectArray::GetGameObject(id, &outPtr) @0x004d8230 → bool.

game_objects[] is an array of handles, not pointers. Initial code treated entries as CSWSObject** and got kind-byte garbage; iteration now resolves each handle through GetGameObject.

## Declarations (in source order)

- L43 `enum class GameObjectKind`: GAME_OBJECT_TYPES at CSWSObject +0x8.
- L59 `void* GetCurrentArea()`: Convenience wrapper around engine_player::GetPlayerArea.
- L65 `int GetObjectKind(void* gameObject)`: -1 on null / fault.
- L69 `uint32_t GetObjectHandle(void* gameObject)`: CGameObject.id @+0x4.
- L77 `void* ResolveServerObjectHandle(uint32_t handle)`: Two independent handle namespaces.
- L83 `void* ResolveClientObjectHandle(uint32_t handle)`: Client-side: AppManager → CClientExoApp → GetGameObject → CSWCObject*, then +0xf8 server_object → CSWSObject*.
- L89 `void* ResolveClientObject(uint32_t handle)`: Same first step WITHOUT the +0xf8 chain — returns the CLIENT object (CSWCObject* / CSWCCreature*) for a high-bit client handle.
- L92 `bool GetObjectPosition(void* gameObject, Vector& out)`: CSWSObject +0x90.
- L97 `void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex)`: CSWSArea::GetRoom @0x4bb600.
- L101 `bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize)`: CSWSArea.name CExoLocString @+0x150 (inline → TLK strref @+0x154); falls back to CSWSArea.tag CExoString @+0x158 (modder-assigned).
- L106 `bool GetRoomDisplayName(void* area, int roomIndex, char* outBuf, size_t bufSize)`: CSWSArea.room_names[index] — CExoString* array, stride 8.
- L120 `bool GetObjectName(void* gameObject, char* outBuf, size_t bufSize)`: Per-kind localized name lookup: Door CSWSDoor.loc_name @+0x39c Creature CSWSCreature.creature_stats @+0xa74 → first_name @+0x14 Placeable CSWSPlaceable.loc_name @+0x228 Item CSWSItem.localized_name @+0x280 Waypoint CS...
- L137 `bool GetObjectBaseName(void* gameObject, char* outBuf, size_t bufSize)`: GetObjectName split in two, for callers that need to put something BETWEEN the name and its suffix.
- L138 `void AppendObjectStateSuffix(void* gameObject, char* outBuf, size_t bufSize)`
- L149 `bool WasDoorLockedAtAreaLoad(void* serverDoor)`: True when this door was locked at the moment the current area's door snapshot was taken — i.e.
- L155 `void ResetDoorLockSnapshot()`: Drops the door lock snapshot; the next query rebuilds it from the live area.
- L162 `bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize)`: CSWSObject.tag CExoString @+0x18 — the modder-assigned, locale-INDEPENDENT object id (e.g.
- L168 `bool GetAreaTag(void* area, char* outBuf, size_t bufSize)`: CSWSArea.tag CExoString @+0x158 — the area's modder-assigned id.
- L176 `bool GetCurrentAreaResName(char* outBuf, size_t bufSize)`: Current module's resource name via CSWSModule::GetModuleResourceName @0x004c4b80 — the stable, language-INDEPENDENT module/area resref (e.g.
- L187 `int ReadGlobalNumber(const char* name)`: One global plot NUMBER (engine stores it as a byte) by name, via CServerExoApp::GetGlobalVariableTable @0x004aee60 + GetValueNumber @0x00529240.
- L197 `bool IsLoadingSaveGame()`: CServerExoApp::GetLoadFromSaveGame @0x004af050 — the engine's own "I am loading a saved game" flag (CServerExoAppInternal.load_from_savegame).
- L209 `void SubstitutePlayerNameToken(char* buf, size_t bufSize)`: Replace the engine's " " player-name token in `buf` with the PC's name, in place.
- L216 `bool GetObjectDisplayNameByHandle(uint32_t handle, char* outBuf, size_t bufSize)`: Wraps CClientExoApp::GetObjectName(ulong, CExoString*) — the engine's universal accessor.
- L228 `bool IsUsablePlaceable(void* placeable)`: Pillar 4 sub-state predicates (refine the kind filter): IsUsablePlaceable CSWSPlaceable.usable @+0x328 OR has_inventory @+0x324.
- L229 `bool IsLandmarkWaypoint(void* waypoint)`
- L230 `bool IsTransitionTrigger(void* trigger)`
- L238 `int GetTriggerGeometry(void* trigger, Vector* out, int maxVerts)`: Copies the trigger's footprint polygon (CSWSTrigger.geometry — Vector array in WORLD space, count @+0x284, data @+0x288) into `out`, up to maxVerts.
- L247 `bool GetObjectLocalBoolean(void* gameObject, int index)`: Reads one bit of the object's fixed NWScript local-boolean table (CSWVarTable @ CSWSObject+0x110, `ulong local_booleans[3]` — the table GetLocalBoolean/SetLocalBoolean script commands hit; bit layout confirmed against...
- L256 `bool IsEmptyContainer(void* gameObject)`: True only when `gameObject` is a loot container (kind == Placeable AND has_inventory != 0) whose CItemRepository currently holds zero items — i.e.
- L260 `bool IsDoorOpen(void* serverDoor)`: CSWSDoor.open_state +0x2cc.
- L264 `bool IsDoorLocked(void* serverDoor)`: CSWSDoor live locked flag (kDoorLockedOffset).
- L269 `bool DoorHasTransitionDest(void* serverDoor)`: True when the door names another area as its destination (non-empty TransitionDestination, the "Taris - Upper City Cantina" in the door's label).
- L275 `enum class LockNeed`: What it takes to open a locked door or container, read from the object's own lock fields (UTD / UTP KeyRequired, KeyName, OpenLockDC).
- L285 `LockNeed ReadLockNeed(void* gameObject, int& outDc, bool* outLocked = nullptr)`: outLocked (optional) receives the live locked flag, so a caller can say "locked" even when the need is Unknown.
- L290 `int GetContainerItemCount(void* gameObject)`: Items currently in a loot container (placeable with HasInventory), the same repository count IsEmptyContainer reads.
- L298 `bool IsDoorStatic(void* serverDoor)`: CSWSDoor Static flag +0x3c0 (UTD Static).
- L306 `bool IsDoorCosmetic(void* serverDoor)`: A door the player can NEVER use — Static, with (on KOTOR 2) no lock evidence to the contrary.
- L316 `bool MaybeDrivePassiveSelection()`: Root-cause fix (2026-07-19, rev 2): the Endar Spire opening HOLDS the global fade at alpha=1.0 (engine re-asserts it every frame, so it can't be cleared) after the fade animation finishes, while the player keeps world...
- L320 `bool IsMapNoteEnabled(void* waypoint)`: CSWSWaypoint.map_note_enabled +0x22c.
- L329 `bool EnableMapNote(void* waypoint)`: Replica of the write path in ExecuteCommandSetMapPinEnabled(TRUE) @0x0054b460: guard on has_map_note (+0x228), then set the int at +0x22c on the SERVER waypoint.
- L334 `void* GetAreaMap()`: AppManager → CServerExoApp → GetModule → CSWSModule.area_map @+0x218.
- L338 `bool IsWorldPointExplored(void* areaMap, const Vector& pos)`: CSWSAreaMap::IsWorldPointExplored @0x00579210.
- L347 `bool GetFogCellSizeM(void* areaMap, float& outCellXM, float& outCellYM)`: Fog-of-war grid cell size in world metres.
- L355 `bool GetMapRotateCCWFromWorldOrientation(void* areaMap, const Vector& orientation, float& outDegCCW)`: CSWSAreaMap::GetMapRotateCCWFromWorldOrientation @0x00578ed0.
- L362 `void* GetClientArea(void* serverArea)`: Back-pointer at CSWSArea +0x2d0.
- L364 `int GetMapPinCount(void* clientArea)`
- L369 `void* GetMapPinAt(void* clientArea, int i)`: Pointer-array semantics (CSWCMapPin**, 4-byte stride, each slot a heap-allocated 0x110-byte pin).
- L373 `bool GetMapPinPosition(void* mapPin, Vector& out)`: CSWCMapPin embeds CSWCObject at +0; position at +0x24 (standard CGameObject offset).
- L385 `bool IsMapPinEnabled(void* mapPin)`: CSWCMapPin.enabled +0xfc.
- L389 `bool GetMapPinNoteText(void* mapPin, char* outBuf, size_t bufSize)`: CSWCMapPin.note_text CExoString @+0x100.
- L406 `bool CreateMapPin(void* clientArea, const Vector& pos, const char* name, uint32_t referenceNumber, void** outPin = nullptr)`: Replicates the engine pattern from HandleServerToPlayerMapPinReferenceNumber @0x652d60: operator_new(0x110) → CSWCMapPin ctor → field writes (pos +0x24, enabled +0xfc, note_text via CExoString::operator= +0x100, flags...
- L414 `bool GetWaypointMapNote(void* waypoint, char* outBuf, size_t bufSize)`: CSWSWaypoint.map_note CExoLocString @+0x230 (strref @+0x234).
- L420 `class AreaObjectIterator`: Snapshots data pointer + size at construction.
- L436 `void ForEachDoor(void* area, Fn fn)`: Calls fn(door, pos) for every door of `area` whose position reads.
- L448 `void* NearestDoor(void* area, const Vector& p, float maxM, Accept accept, Vector* outPos = nullptr)`: The door of `area` nearest to `p` in the plane, within `maxM`, for which accept(door) holds; null if none.
- L765 `bool TrapDetectedByAnyOf(void* gameObject, const uint32_t* ids, int idCount)`: True when `gameObject`'s trap has been detected by any of the given SERVER object ids (pass the current party's ids).
- L790 `int BuildAreaWallCache(void* area, WallEdge* outBuf, int maxEdges)`: Walks every room in `area`, then every face in each room's CSWRoomSurfaceMesh, and emits one WallEdge for each triangle side whose adjacency is `-1` (perimeter / wall sentinel).
- L797 `int BuildAreaFloorCache(void* area, FloorTri* outBuf, int maxTris)`: Walks every room's surface mesh and emits every face as a FloorTri.

Also 84 constants or variables at namespace scope.
