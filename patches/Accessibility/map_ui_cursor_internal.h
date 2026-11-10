// Internal to the map_ui_cursor module (map_ui_cursor*.cpp); no other file includes
// it. The public interface is map_ui_cursor.h. Here: what the files share.

#pragma once

#include "map_ui_cursor.h"

#include <windows.h>
#include <string>

#include "log.h"
#include "room_topology.h"
#include "view_mode.h"

namespace acc::map_ui_cursor {

// The hover-pause cadence, shared with view mode so both cursors speak at
// the same pace.
using acc::view_mode::kHoverPauseMs;

// The map-pixel bounds. The engine's own checks are `< 0x1b9` / `< 0x101`,
// so the highest legal pixel is (440, 256).
constexpr int kMapPixelMaxX = 440;
constexpr int kMapPixelMaxY = 256;

// What the cursor says when it is not over a note, in order of precedence:
// fog of war (Unexplored), a Bioware-authored landmark near the cursor
// (Landmark), a mod-supplied room name (RoomName; vanilla resref-style room
// ids are filtered out), the room_topology shape of the region
// (TerrainShape: corridor, room, junction), or nothing (None).
enum class AmbientKind {
    None,
    Unexplored,
    Landmark,
    RoomName,
    TerrainShape,
};

struct CursorState {
    bool   active       = false;
    float  px           = kMapPixelMaxX / 2.0f;
    float  py           = kMapPixelMaxY / 2.0f;
    Vector world        = {0, 0, 0};
    DWORD  last_tick_ms = 0;
    bool   at_edge      = false;   // "edge of the map" said for this push

    // Note hover pause, keyed by pointer identity: a waypoint, a user map
    // pin or a shipped hint. pending_note_is_pin picks the pin's text
    // accessor at speak time.
    void*  pending_note            = nullptr;
    bool   pending_note_is_pin     = false;
    DWORD  pending_note_started_ms = 0;
    void*  last_spoken_note        = nullptr;

    // Ambient hover pause: one timer for all kinds, so a flip between kinds
    // does not debounce twice. The cursor stays silent inside the zone it
    // last spoke and re-arms on a different kind or key (see
    // TickAmbientHover for the key per kind); last_spoken_ambient_text
    // also counts two zones with the same label as one.
    AmbientKind pending_ambient_kind       = AmbientKind::None;
    int         pending_ambient_key        = -1;
    DWORD       pending_ambient_started_ms = 0;
    AmbientKind last_spoken_ambient_kind   = AmbientKind::None;
    int         last_spoken_ambient_key    = -1;
    std::string last_spoken_ambient_text;
    // TerrainShape's label, stashed when the pause is armed; room_topology
    // builds it once per cluster, so it is stable across the cluster.
    std::string pending_shape_text;
};

extern CursorState g_state;

// The room_topology shape at the cursor, probed once per tick; never for a
// cell the player has not explored.
struct Shape {
    bool        have      = false;
    std::string text;
    int         clusterId = acc::room_topology::kClusterIdNone;
};

// The nearest hover candidate of one kind: obj is null when none is within
// the hover radius; scanned is for the log.
struct Nearest {
    void* obj     = nullptr;
    float dist2   = 0.0f;
    int   scanned = 0;
};

// Helpers shared across the files, by the file that defines them.

// map_ui_cursor.cpp
std::string WithShape(const std::string& text, const Shape& shape);

// map_ui_cursor_transform.cpp
bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy);
bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld);

// map_ui_cursor_hover.cpp
Nearest FindNearestExploredMapNote(void* mapPanel, void* areaMap,
                                   float cursorPx, float cursorPy,
                                   acclog::BlockLog& dump);
Nearest FindNearestUserMapPin(void* clientArea, void* areaMap,
                              float cursorPx, float cursorPy);
Nearest FindNearestShippedHint(void* areaMap, float cursorPx, float cursorPy);

// map_ui_cursor_ambient.cpp
void ClearPendingAmbient();
void ClearLastSpokenAmbient();
void TickAmbientHover(void* areaMap, const Shape& shape, DWORD now);

}  // namespace acc::map_ui_cursor
