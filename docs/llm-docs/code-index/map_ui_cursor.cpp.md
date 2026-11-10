# map_ui_cursor.cpp (784 lines)

Source: `patches/Accessibility/map_ui_cursor.cpp`

Phase 5 lay-off 6 — virtual cursor on the area map UI.

See map_ui_cursor.h for the user model and engine surfaces.

Cursor lives in map-pixel space [0..440] × [0..256]. Movement keys translate the cursor in pixel space (with map-frame-relative axes — W = "up on the map", S = "down", A = "left", D = "right"; this is a 2D scan UI, not a 3D heading). The cursor's world projection comes from inverting CSWSAreaMap's pixel transform (4 fields, see docs/navsystems-investigation.md §Q4).

## Declarations (in source order)

- L115 `bool IsForegroundProcess()`: Foreground-window gate.
- L127 `bool IsMapPanelActive(void** outPanel)`: The InGameMap sub-screen is pushed onto panels[] *under* the CSWGuiInGameMenu strip — GetForegroundPanel returns the strip, not the map.
- L133 `void SeedCursorAtPlayer(void* areaMap)`
- L147 `void ResetSessionState()`
- L164 `bool KeyDown(int vk)`
- L181 `bool AxisDown(acc::engine_keymap::MoveAxis axis)`: True if the key bound to the given movement direction is held.
- L188 `bool IsActive()`
- L190 `bool TryGetCursorWorldPosition(Vector& out)`
- L196 `void PanToWorld(const Vector& world, void* suppressWaypoint)`
- L248 `void Tick()`
