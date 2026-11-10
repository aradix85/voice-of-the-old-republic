# map_ui_cursor_transform.cpp (108 lines)

Source: `patches/Accessibility/map_ui_cursor_transform.cpp`

The area map's pixel transform: world position to map pixel and back, as CSWSAreaMap computes it.

## Declarations (in source order)

- L36 `bool ReadAreaMapTransform(void* areaMap, uint32_t& outOrientation, float& outWuPerPx, float& outWuPerPy, float& outOriginX, float& outOriginY)`: Read CSWSAreaMap pixel-transform fields.
- L59 `void ApplyOrientationForward(uint32_t orient, float& xw, float& yw)`: Apply the engine's forward rotation per orientation, matching CSWSAreaMap::GetMapPixelFromWorldCoord exactly.
- L71 `void ApplyOrientationInverse(uint32_t orient, float& xw, float& yw)`: Undo it.
- L84 `bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy)`
- L95 `bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld)`
