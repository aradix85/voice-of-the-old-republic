# engine_area_map.cpp (437 lines)

Source: `patches/Accessibility/engine_area_map.cpp`

area map surface: fog-of-war, map notes and map pins.

Split out of engine_area.cpp by the Phase-1 structure pass (refactoring candidate 3). engine_area.cpp kept the core object model (iteration, handle resolution, naming/tagging, door/waypoint/trigger state); this file owns everything that talks to CSWSAreaMap / CSWCArea's map-pin list, plus the two module-scope readers (ReadGlobalNumber, IsLoadingSaveGame) that only the map paths use.

Declarations stay in engine_area.h - splitting that header is a separate, deferred item, so every existing includer is unaffected.

## Declarations (in source order)

- L29 `bool IsMapNoteEnabled(void* waypoint)`
- L40 `bool EnableMapNote(void* waypoint)`
- L56 `bool GetWaypointMapNote(void* waypoint, char* outBuf, size_t bufSize)`
- L107 `void* GetAreaMap()`
- L122 `bool GetCurrentAreaResName(char* outBuf, size_t bufSize)`
- L156 `int ReadGlobalNumber(const char* name)`
- L193 `bool IsLoadingSaveGame()`
- L215 `bool IsWorldPointExplored(void* areaMap, const Vector& pos)`
- L226 `bool GetFogCellSizeM(void* areaMap, float& outCellXM, float& outCellYM)`
- L250 `bool GetMapRotateCCWFromWorldOrientation(void* areaMap, const Vector& orientation, float& outDegCCW)`
- L265 `void* GetClientArea(void* serverArea)`
- L296 `int GetMapPinCount(void* clientArea)`
- L307 `void* GetMapPinAt(void* clientArea, int i)`
- L320 `bool GetMapPinPosition(void* mapPin, Vector& out)`
- L332 `bool IsMapPinEnabled(void* mapPin)`
- L343 `bool GetMapPinNoteText(void* mapPin, char* outBuf, size_t bufSize)`
- L384 `bool CreateMapPin(void* clientArea, const Vector& pos, const char* name, uint32_t referenceNumber, void** outPin)`
