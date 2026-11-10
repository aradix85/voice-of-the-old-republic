// Map UI virtual cursor.
//
// While CSWGuiInGameMap is open, the player's move and turn keys (the
// primary binds, W/A/S/D by default) pan a cursor in map pixel space instead
// of moving the character. After the hover pause it speaks what sits
// underneath, in priority:
//
//   1. The nearest explored map note, user marker or shipped hint within the
//      hover radius.
//   2. Fog of war: "Unexplored", never the name underneath.
//   3. A landmark near the cursor.
//   4. A mod-supplied friendly room name (vanilla resref-style room ids
//      never reach speech).
//   5. The shape of the region (room_topology): corridor, room, junction.
//
// Notes, landmarks and room names get the region shape after them.
// Distinct from the engine's prev/next-note cycle (up/down buttons), which
// steps through curated landmarks; the cursor pans continuously.
//
// The world position inverts CSWSAreaMap::GetMapPixelFromWorldCoord inline
// (map_ui_cursor_transform.cpp); the fog gate is
// CSWSAreaMap::IsWorldPointExplored (engine_area).

#pragma once

#include "engine_offsets.h"

namespace acc::map_ui_cursor {

// Self-gates on foreground window + PanelKind::InGameMap present +
// CSWSAreaMap resolvable; idle when any gate fails. Run after the menu
// monitors so the panel snapshot is current.
void Tick();

// The cursor's world position; false when the map cursor is not active.
bool TryGetCursorWorldPosition(Vector& out);

// suppressWaypoint = the waypoint just announced via cycle, to avoid
// double-speak on the next hover-pause. Pass nullptr for non-waypoint
// pans (cursor falls through to terrain-shape narration).
void PanToWorld(const Vector& world, void* suppressWaypoint);

}  // namespace acc::map_ui_cursor
