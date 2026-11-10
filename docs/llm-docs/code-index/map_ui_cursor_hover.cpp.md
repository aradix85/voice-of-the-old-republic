# map_ui_cursor_hover.cpp (170 lines)

Source: `patches/Accessibility/map_ui_cursor_hover.cpp`

What is under the map cursor: the nearest explored map note, user marker or shipped hint within the hover radius.

## Declarations (in source order)

- L46 `void Consider(Nearest& best, void* obj, float ppx, float ppy, float cursorPx, float cursorPy)`: Takes `obj` as the best candidate when its map pixel is nearer the cursor than the best so far, which starts at the hover radius.
- L66 `Nearest FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, acclog::BlockLog& dump)`: The nearest explored map note: the map hider's waypoint list under the filter the engine's GetNext/PrevMapNote applies (map note enabled, position explored), but by distance rather than list order.
- L131 `Nearest FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy)`: The nearest of the player's own markers in CSWCArea.map_pins[].
- L154 `Nearest FindNearestShippedHint(void* areaMap, float cursorPx, float cursorPy)`: The nearest of the mod's shipped curated hints (map_shipped_hints: table rows, no engine object).
