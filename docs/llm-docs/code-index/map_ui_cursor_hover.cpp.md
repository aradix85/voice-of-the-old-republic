# map_ui_cursor_hover.cpp (289 lines)

Source: `patches/Accessibility/map_ui_cursor_hover.cpp`

What is under the map cursor: the nearest explored map note, user marker or shipped hint within the hover radius, and the waypoint's note text and tag.

## Declarations (in source order)

- L80 `void* FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount, float* outBestDist2, acclog::BlockLog* dump)`: Walk the map-hider's waypoint linked list, find the nearest waypoint whose map-pixel projection lands within kHoverHitRadiusPx of the cursor AND whose has_map_note != 0 AND whose position IsWorldPointExplored == true.
- L194 `void* FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount, float* outBestDist2)`: Hit-test against the mod's own saved markers in `CSWCArea.map_pins[]`.
- L237 `const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint( void* areaMap, float cursorPx, float cursorPy, int* outScannedCount, float* outBestDist2)`: Hit-test the mod's shipped curated hints (map_shipped_hints) — static table rows with no engine object behind them.
- L268 `bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize)`
- L275 `bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize)`: Read CSWSWaypoint.Tag for diagnostic logging.
