# map_ui_cursor_ambient.cpp (174 lines)

Source: `patches/Accessibility/map_ui_cursor_ambient.cpp`

What the map cursor says when it is not over a note: the ambient under it (see AmbientKind), after the hover pause and once per zone.

## Declarations (in source order)

- L30 `const char* AmbientKindStr(AmbientKind k)`
- L43 `Ambient Classify(void* areaMap, const Shape& shape)`: Fog wins outright: never a landmark or room name for a cell the player has not revealed, although the engine has both at hand.
- L79 `std::string AmbientText(const Ambient& a, const std::string& shapeText)`: The text for an ambient.
- L102 `void ClearPendingAmbient()`
- L109 `void ClearLastSpokenAmbient()`
- L115 `void TickAmbientHover(void* areaMap, const Shape& shape, DWORD now)`
