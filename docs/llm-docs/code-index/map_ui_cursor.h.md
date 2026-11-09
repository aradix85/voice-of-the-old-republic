# map_ui_cursor.h (49 lines)

Source: `patches/Accessibility/map_ui_cursor.h`

Map UI virtual cursor.

While CSWGuiInGameMap is foreground, W/A/S/D pan a virtual cursor in map pixel space instead of moving the character. 300ms hover-pause speaks whatever sits underneath, in priority:

1. Explored map-note waypoint (CSWSWaypoint.map_note_enabled != 0). 2. Fog-of-war (CSWSAreaMap::IsWorldPointExplored false) — speaks "Unexplored", never the room name underneath. 3. Explored room with a cached landmark — landmark text. 4. Explored room with a mod-supplied friendly room_names entry. 5. Silent — vanilla resref-style ids never reach speech.

Distinct from the engine's prev/next-note cycle (up/down buttons). Cycle steps through curated landmarks; cursor pans continuously.

Engine surfaces: CSWSAreaMap (Module+0x218): +0x10 orientation, +0x18/+0x1c units-per-pixel, +0x20/+0x24 origin. CSWSAreaMap::GetMapPixelFromWorldCoord @0x00578e00 — we invert this inline (no engine call on the cursor hot path). CSWSAreaMap::IsWorldPointExplored @0x00579210 — fog gate. CServerExoApp::GetModule @0x004ae6b0.

## Declarations (in source order)

- L35 `void Tick()`: Self-gates on foreground window + PanelKind::InGameMap foreground + CSWSAreaMap resolvable.
- L39 `bool IsActive()`: True iff the cursor is consuming W/A/S/D.
- L41 `bool TryGetCursorWorldPosition(Vector& out)`
- L46 `void PanToWorld(const Vector& world, void* suppressWaypoint)`: suppressWaypoint = the waypoint just announced via cycle, to avoid double-speak on the next hover-pause.
