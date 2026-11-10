# map_ui_cursor_ambient.cpp (66 lines)

Source: `patches/Accessibility/map_ui_cursor_ambient.cpp`

What the map cursor says when it is not over a note: the ambient kinds and their text.

## Declarations (in source order)

- L10 `const char* AmbientKindStr(AmbientKind k)`
- L27 `std::string ResolveAmbientText(AmbientKind kind, int roomIdx, const char* landmarkBuf, const char* shapeText)`: Render the speak-text for an ambient kind into outBuf.
