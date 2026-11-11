# engine_area_map.cpp (403 lines)

Source: `patches/Accessibility/engine_area_map.cpp`

area map surface: fog-of-war, map notes and map pins.

Split out of engine_area.cpp by the Phase-1 structure pass (refactoring candidate 3). engine_area.cpp kept the core object model (iteration, handle resolution, naming/tagging, door/waypoint/trigger state); this file owns everything that talks to CSWSAreaMap / CSWCArea's map-pin list, plus the two module-scope readers (ReadGlobalNumber, IsLoadingSaveGame) that only the map paths use.

Declarations stay in engine_area.h - splitting that header is a separate, deferred item, so every existing includer is unaffected.

## Declarations (in source order)

- L29 `bool IsMapNoteEnabled(void* waypoint)`
- L33 `bool EnableMapNote(void* waypoint)`
- L49 `bool GetWaypointMapNote(void* waypoint, char* outBuf, size_t bufSize)`
- L100 `void* GetAreaMap()`
- L115 `bool GetCurrentAreaResName(char* outBuf, size_t bufSize)`
- L149 `int ReadGlobalNumber(const char* name)`
- L186 `bool IsLoadingSaveGame()`
- L208 `bool IsWorldPointExplored(void* areaMap, const Vector& pos)`
- L219 `bool GetFogCellSizeM(void* areaMap, float& outCellXM, float& outCellYM)`
- L243 `bool GetMapRotateCCWFromWorldOrientation(void* areaMap, const Vector& orientation, float& outDegCCW)`
- L258 `void* GetClientArea(void* serverArea)`
- L283 `int GetMapPinCount(void* clientArea)`
- L287 `void* GetMapPinAt(void* clientArea, int i)`
- L300 `bool GetMapPinPosition(void* mapPin, Vector& out)`
- L305 `bool IsMapPinEnabled(void* mapPin)`
- L309 `bool GetMapPinNoteText(void* mapPin, char* outBuf, size_t bufSize)`
- L350 `bool CreateMapPin(void* clientArea, const Vector& pos, const char* name, uint32_t referenceNumber, void** outPin)`
