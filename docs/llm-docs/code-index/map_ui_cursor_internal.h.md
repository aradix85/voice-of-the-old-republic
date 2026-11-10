# map_ui_cursor_internal.h (111 lines)

Source: `patches/Accessibility/map_ui_cursor_internal.h`

Internal to the map_ui_cursor module (map_ui_cursor*.cpp); no other file includes it. The public interface is map_ui_cursor.h. Here: what the files share.

## Declarations (in source order)

- L30 `enum class AmbientKind`: What the cursor says when it is not over a note, in order of precedence: fog of war (Unexplored), a Bioware-authored landmark near the cursor (Landmark), a mod-supplied room name (RoomName; vanilla resref-style room i...
- L38 `struct CursorState`
- L74 `struct Shape`: The room_topology shape at the cursor, probed once per tick; never for a cell the player has not explored.
- L82 `struct Nearest`: The nearest hover candidate of one kind: obj is null when none is within the hover radius; scanned is for the log.
- L91 `std::string WithShape(const std::string& text, const Shape& shape)`: map_ui_cursor.cpp
- L94 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`: map_ui_cursor_transform.cpp
- L95 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
- L98 `Nearest FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, acclog::BlockLog& dump)`: map_ui_cursor_hover.cpp
- L101 `Nearest FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy)`
- L103 `Nearest FindNearestShippedHint(void* areaMap, float cursorPx, float cursorPy)`
- L106 `void ClearPendingAmbient()`: map_ui_cursor_ambient.cpp
- L107 `void ClearLastSpokenAmbient()`
- L108 `void TickAmbientHover(void* areaMap, const Shape& shape, DWORD now)`

Also 4 constants or variables at namespace scope.
