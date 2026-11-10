# map_ui_cursor.cpp (374 lines)

Source: `patches/Accessibility/map_ui_cursor.cpp`

Map UI cursor: the session, the move keys, PanToWorld and the per-tick flow (move the cursor, find what is under it, speak it after the hover pause).

The cursor lives in map-pixel space [0..440] x [0..256]. The move keys translate it with map-frame axes (forward = up on the map): a 2D scan UI, not a 3D heading.

## Declarations (in source order)

- L39 `bool IsForegroundProcess()`: Keys are only consumed while KOTOR has focus, so alt-tabbing does not steal movement from other apps.
- L54 `bool AxisDown(acc::engine_keymap::MoveAxis axis)`: True if the PRIMARY key bound to the movement axis is held: the map cursor follows the keys the player walks with (forward → up, back → down, strafe-left → left, strafe-right → right), WASD by default.
- L60 `bool ClampToMap(float& px, float& py)`: Clamps a map pixel into the map; true if it had to.
- L69 `void SeedCursorAtPlayer(void* areaMap)`
- L81 `void ResetSessionState()`: A fresh session state; the cursor keeps its position.
- L91 `void Deactivate(const char* why)`
- L97 `void ClearPendingNote()`
- L105 `void StartSession(void* areaMap, DWORD now)`: Session start: seed the cursor at the player, speak the area name, and make sure the region shapes are built.
- L134 `bool MoveCursor(void* areaMap, DWORD now)`: Moves the cursor by the held move keys; true if it moved.
- L181 `void* FindHoverHit(void* mapPanel, void* areaMap, bool moved, bool& outIsPin)`: The note under the cursor: the nearest map note, user marker or shipped hint, the closest of the three.
- L220 `Shape ProbeShape(void* areaMap)`: The region shape at the cursor, from the source the in-world walking adapter speaks from, so cursor and walking agree on corridor, junction or dead end.
- L236 `void SpeakNote(void* hit, bool isPin, const Shape& shape)`: Speaks the note under the cursor: its text, or a generic name when it has none (an enabled note the engine draws must not stay silent), then the region shape.
- L268 `void TickNoteHover(void* hit, bool hitIsPin, const Shape& shape, DWORD now)`: The hover pause on a note: arm on a new note, speak once it has held for kHoverPauseMs, then stay silent on it until the cursor leaves.
- L296 `std::string WithShape(const std::string& text, const Shape& shape)`
- L302 `bool TryGetCursorWorldPosition(Vector& out)`
- L308 `void PanToWorld(const Vector& world, void* suppressWaypoint)`
- L339 `void Tick()`
