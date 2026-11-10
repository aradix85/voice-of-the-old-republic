# map_ui_cursor_internal.h (64 lines)

Source: `patches/Accessibility/map_ui_cursor_internal.h`

Internal to the map_ui_cursor module (map_ui_cursor*.cpp); no other file includes it. The public interface is map_ui_cursor.h. Here: what the files share.

## Declarations (in source order)

- L34 `enum class AmbientKind`: GetAreaMap / IsWorldPointExplored entry points moved to engine_area.
- L49 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`: map_ui_cursor_transform.cpp
- L50 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
- L53 `void* FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr, acclog::BlockLog* dump = nullptr)`: map_ui_cursor_hover.cpp
- L54 `void* FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`
- L55 `const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint( void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr)`
- L56 `bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize)`
- L57 `bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize)`
- L60 `const char* AmbientKindStr(AmbientKind k)`: map_ui_cursor_ambient.cpp
- L61 `std::string ResolveAmbientText(AmbientKind kind, int roomIdx, const char* landmarkBuf, const char* shapeText)`
