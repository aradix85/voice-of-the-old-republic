# map_ui_cursor.cpp (1247 lines)

Source: `patches/Accessibility/map_ui_cursor.cpp`

Phase 5 lay-off 6 — virtual cursor on the area map UI.

See map_ui_cursor.h for the user model and engine surfaces.

Cursor lives in map-pixel space [0..440] × [0..256]. Movement keys translate the cursor in pixel space (with map-frame-relative axes — W = "up on the map", S = "down", A = "left", D = "right"; this is a 2D scan UI, not a 3D heading). The cursor's world projection comes from inverting CSWSAreaMap's pixel transform (4 fields, see docs/navsystems-investigation.md §Q4).

## Declarations (in source order)

- L222 `bool IsForegroundProcess()`: Foreground-window gate.
- L234 `bool IsMapPanelActive(void** outPanel)`: The InGameMap sub-screen is pushed onto panels[] *under* the CSWGuiInGameMenu strip — GetForegroundPanel returns the strip, not the map.
- L247 `bool ReadAreaMapTransform(void* areaMap, uint32_t& outOrientation, float& outWuPerPx, float& outWuPerPy, float& outOriginX, float& outOriginY)`: Read CSWSAreaMap pixel-transform fields.
- L270 `void ApplyOrientationForward(uint32_t orient, float& xw, float& yw)`: Apply the engine's forward rotation per orientation, matching CSWSAreaMap::GetMapPixelFromWorldCoord exactly.
- L282 `void ApplyOrientationInverse(uint32_t orient, float& xw, float& yw)`: Undo it.
- L293 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`
- L304 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
- L332 `void* FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr, acclog::BlockLog* dump = nullptr)`: Walk the map-hider's waypoint linked list, find the nearest waypoint whose map-pixel projection lands within kHoverHitRadiusPx of the cursor AND whose has_map_note != 0 AND whose position IsWorldPointExplored == true.
- L446 `void* FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`: Hit-test against the mod's own saved markers in `CSWCArea.map_pins[]`.
- L489 `const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint( void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`: Hit-test the mod's shipped curated hints (map_shipped_hints) — static table rows with no engine object behind them.
- L520 `bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize)`
- L524 `void SeedCursorAtPlayer(void* areaMap)`
- L538 `void ResetSessionState()`
- L555 `const char* AmbientKindStr(AmbientKind k)`
- L572 `std::string ResolveAmbientText(AmbientKind kind, int roomIdx, const char* landmarkBuf, const char* shapeText)`: Render the speak-text for an ambient kind into outBuf.
- L614 `bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize)`: Read CSWSWaypoint.Tag for diagnostic logging.
- L627 `bool KeyDown(int vk)`
- L644 `bool AxisDown(acc::engine_keymap::MoveAxis axis)`: True if the key bound to the given movement direction is held.
- L651 `bool IsActive()`
- L653 `bool TryGetCursorWorldPosition(Vector& out)`
- L659 `void PanToWorld(const Vector& world, void* suppressWaypoint)`
- L711 `void Tick()`
