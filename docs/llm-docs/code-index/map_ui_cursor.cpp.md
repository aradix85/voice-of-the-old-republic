# map_ui_cursor.cpp (1268 lines)

Source: `patches/Accessibility/map_ui_cursor.cpp`

Phase 5 lay-off 6 — virtual cursor on the area map UI.

See map_ui_cursor.h for the user model and engine surfaces.

Cursor lives in map-pixel space [0..440] × [0..256]. Movement keys translate the cursor in pixel space (with map-frame-relative axes — W = "up on the map", S = "down", A = "left", D = "right"; this is a 2D scan UI, not a 3D heading). The cursor's world projection comes from inverting CSWSAreaMap's pixel transform (4 fields, see docs/navsystems-investigation.md §Q4).

## Declarations (in source order)

- L224 `bool IsForegroundProcess()`: Foreground-window gate.
- L236 `bool IsMapPanelActive(void** outPanel)`: The InGameMap sub-screen is pushed onto panels[] *under* the CSWGuiInGameMenu strip — GetForegroundPanel returns the strip, not the map.
- L249 `bool ReadAreaMapTransform(void* areaMap, uint32_t& outOrientation, float& outWuPerPx, float& outWuPerPy, float& outOriginX, float& outOriginY)`: Read CSWSAreaMap pixel-transform fields.
- L272 `void ApplyOrientationForward(uint32_t orient, float& xw, float& yw)`: Apply the engine's forward rotation per orientation, matching CSWSAreaMap::GetMapPixelFromWorldCoord exactly.
- L284 `void ApplyOrientationInverse(uint32_t orient, float& xw, float& yw)`: Undo it.
- L295 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`
- L306 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
- L334 `void* FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr, acclog::BlockLog* dump = nullptr)`: Walk the map-hider's waypoint linked list, find the nearest waypoint whose map-pixel projection lands within kHoverHitRadiusPx of the cursor AND whose has_map_note != 0 AND whose position IsWorldPointExplored == true.
- L448 `void* FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`: Hit-test against the mod's own saved markers in `CSWCArea.map_pins[]`.
- L491 `const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint( void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`: Hit-test the mod's shipped curated hints (map_shipped_hints) — static table rows with no engine object behind them.
- L522 `bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize)`
- L526 `void SeedCursorAtPlayer(void* areaMap)`
- L540 `void ResetSessionState()`
- L557 `const char* AmbientKindStr(AmbientKind k)`
- L574 `std::string ResolveAmbientText(AmbientKind kind, int roomIdx, const char* landmarkBuf, const char* shapeText)`: Render the speak-text for an ambient kind into outBuf.
- L616 `bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize)`: Read CSWSWaypoint.Tag for diagnostic logging.
- L629 `bool KeyDown(int vk)`
- L646 `bool AxisDown(acc::engine_keymap::MoveAxis axis)`: True if the key bound to the given movement direction is held.
- L653 `bool IsActive()`
- L655 `bool TryGetCursorWorldPosition(Vector& out)`
- L661 `void PanToWorld(const Vector& world, void* suppressWaypoint)`
- L713 `void Tick()`
