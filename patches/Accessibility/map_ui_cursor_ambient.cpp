// What the map cursor says when it is not over a note: the ambient under it
// (see AmbientKind), after the hover pause and once per zone.

#include "map_ui_cursor_internal.h"

#include "engine_area.h"
#include "speech.h"
#include "strings.h"
#include "transitions.h"

namespace acc::map_ui_cursor {

namespace {

// How near a landmark must be to name the spot under the cursor.
constexpr float kLandmarkRangeM = 15.0f;

// The ambient at the cursor and its identity key: the landmark's index in
// the transitions cache, the .lyt room index, or the room_topology cluster
// id (stable across a cluster, so a curved corridor reads as one zone); -1
// for fog, whose cells do not follow any partition, so all fog is one zone.
// The kind tells the three key spaces apart.
struct Ambient {
    AmbientKind kind    = AmbientKind::None;
    int         key     = -1;
    int         roomIdx = -1;
    char        landmark[128] = {};
};

const char* AmbientKindStr(AmbientKind k) {
    switch (k) {
        case AmbientKind::None:         return "none";
        case AmbientKind::Unexplored:   return "unexplored";
        case AmbientKind::Landmark:     return "landmark";
        case AmbientKind::RoomName:     return "room_name";
        case AmbientKind::TerrainShape: return "terrain_shape";
    }
    return "?";
}

// Fog wins outright: never a landmark or room name for a cell the player has
// not revealed, although the engine has both at hand.
Ambient Classify(void* areaMap, const Shape& shape) {
    Ambient a;
    if (!acc::engine::IsWorldPointExplored(areaMap, g_state.world)) {
        a.kind = AmbientKind::Unexplored;
        return a;
    }
    Vector landmarkPos;
    int landmarkIdx = -1;
    if (acc::transitions::FindLandmarkNear(g_state.world, kLandmarkRangeM,
                                           a.landmark, sizeof(a.landmark),
                                           landmarkPos, &landmarkIdx) &&
        a.landmark[0] != '\0') {
        a.kind = AmbientKind::Landmark;
        a.key  = landmarkIdx;
        return a;
    }
    void* area = acc::engine::GetCurrentArea();
    if (area) acc::engine::GetRoomAtIndexed(area, g_state.world, a.roomIdx);
    if (a.roomIdx >= 0) {
        char room[128] = {0};
        if (acc::engine::GetRoomDisplayName(area, a.roomIdx, room, sizeof(room)) &&
            room[0] != '\0' && !acc::transitions::IsResrefStyleRoomName(room)) {
            a.kind = AmbientKind::RoomName;
            a.key  = a.roomIdx;
            return a;
        }
    }
    if (shape.have) {
        a.kind = AmbientKind::TerrainShape;
        a.key  = shape.clusterId;
    }
    return a;
}

// The text for an ambient. TerrainShape takes shapeText: this tick's shape
// for the zone comparison, the label stashed at arm time for speech.
std::string AmbientText(const Ambient& a, const std::string& shapeText) {
    switch (a.kind) {
        case AmbientKind::Unexplored:
            return acc::strings::Get(acc::strings::Id::MapCursorUnexplored);
        case AmbientKind::Landmark:
            return a.landmark;
        case AmbientKind::RoomName: {
            void* area = acc::engine::GetCurrentArea();
            if (!area) break;
            char room[160] = {0};
            acc::engine::GetRoomDisplayName(area, a.roomIdx, room, sizeof(room));
            return room;
        }
        case AmbientKind::TerrainShape:
            return shapeText;
        case AmbientKind::None:
            break;
    }
    return std::string();
}

}  // namespace

void ClearPendingAmbient() {
    g_state.pending_ambient_kind       = AmbientKind::None;
    g_state.pending_ambient_key        = -1;
    g_state.pending_ambient_started_ms = 0;
    g_state.pending_shape_text.clear();
}

void ClearLastSpokenAmbient() {
    g_state.last_spoken_ambient_kind = AmbientKind::None;
    g_state.last_spoken_ambient_key  = -1;
    g_state.last_spoken_ambient_text.clear();
}

void TickAmbientHover(void* areaMap, const Shape& shape, DWORD now) {
    const Ambient cur = Classify(areaMap, shape);
    if (cur.kind == AmbientKind::None) {
        // Nothing to say; the next named ambient is announced afresh.
        ClearPendingAmbient();
        ClearLastSpokenAmbient();
        return;
    }

    // Same zone as the last one spoken: the same kind and key, or the same
    // text, so neighbouring regions with one label ("junction, north,
    // east") count as one.
    const std::string curText = AmbientText(cur, shape.text);
    const bool sameAsLastSpoken =
        cur.kind == g_state.last_spoken_ambient_kind &&
        (cur.key == g_state.last_spoken_ambient_key ||
         (!curText.empty() && curText == g_state.last_spoken_ambient_text));
    if (sameAsLastSpoken) {
        ClearPendingAmbient();
        return;
    }

    if (cur.kind != g_state.pending_ambient_kind ||
        cur.key != g_state.pending_ambient_key) {
        // A new zone: arm the hover pause.
        g_state.pending_ambient_kind       = cur.kind;
        g_state.pending_ambient_key        = cur.key;
        g_state.pending_ambient_started_ms = now;
        g_state.pending_shape_text =
            cur.kind == AmbientKind::TerrainShape ? shape.text : std::string();
        return;
    }
    if (g_state.pending_ambient_started_ms == 0 ||
        now - g_state.pending_ambient_started_ms < kHoverPauseMs) {
        return;
    }

    const std::string text = AmbientText(cur, g_state.pending_shape_text);
    if (!text.empty()) {
        // A landmark or room name gets the shape after it; TerrainShape is
        // the shape, and fog never exposes the layout.
        const bool withShape = cur.kind == AmbientKind::Landmark ||
                               cur.kind == AmbientKind::RoomName;
        const std::string line = withShape ? WithShape(text, shape) : text;
        acc::speech::Speak(line.c_str(), /*interrupt=*/true);
        acclog::Write("MapCursor",
                      "speak ambient kind=%s key=%d text=\"%s\" shape=\"%s\" "
                      "cursor=(%.1f,%.1f)",
                      AmbientKindStr(cur.kind), cur.key, text.c_str(),
                      shape.have ? shape.text.c_str() : "",
                      g_state.px, g_state.py);
    }
    g_state.last_spoken_ambient_kind = cur.kind;
    g_state.last_spoken_ambient_key  = cur.key;
    g_state.last_spoken_ambient_text = curText;
    ClearPendingAmbient();
}

}  // namespace acc::map_ui_cursor
