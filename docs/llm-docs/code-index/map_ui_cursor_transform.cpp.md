# map_ui_cursor_transform.cpp (87 lines)

Source: `patches/Accessibility/map_ui_cursor_transform.cpp`

The area map's pixel transform: world position to map pixel and back, as CSWSAreaMap::GetMapPixelFromWorldCoord computes it. Inverted inline, so the cursor's hot path makes no engine call.

## Declarations (in source order)

- L15 `bool ReadAreaMapTransform(void* areaMap, uint32_t& outOrientation, float& outWuPerPx, float& outWuPerPy, float& outOriginX, float& outOriginY)`: The CSWSAreaMap transform fields (engine_area.h).
- L38 `void ApplyOrientationForward(uint32_t orient, float& xw, float& yw)`: The engine's forward rotation per orientation, as CSWSAreaMap::GetMapPixelFromWorldCoord decompiles: case 1: (xw, yw) -> (-xw, -yw) case 2: (xw, yw) -> (yw, -xw) case 3: (xw, yw) -> (-yw, xw)
- L50 `void ApplyOrientationInverse(uint32_t orient, float& xw, float& yw)`: Undo it.
- L63 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`
- L74 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
