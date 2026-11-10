// What the map cursor says when it is not over a note: the ambient kinds and their text.

#include "map_ui_cursor_internal.h"

#include "engine_area.h"
#include "strings.h"

namespace acc::map_ui_cursor {

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

// Render the speak-text for an ambient kind into outBuf. Source for the
// TerrainShape branch differs by call site — the dedup-fingerprint pass
// uses the current frame's freshly-computed shape, while the
// hover-pause speak pass uses the stashed pending_shape_text so the
// rendered label stays stable across the arm→speak window.
// Returns true iff outBuf got a non-empty string.
std::string ResolveAmbientText(AmbientKind kind, int roomIdx,
                               const char* landmarkBuf,
                               const char* shapeText) {
    switch (kind) {
        case AmbientKind::Unexplored: {
            const char* t = acc::strings::Get(
                acc::strings::Id::MapCursorUnexplored);
            if (t) return std::string(t);
            break;
        }
        case AmbientKind::Landmark:
            if (landmarkBuf && landmarkBuf[0] != '\0') {
                return std::string(landmarkBuf);
            }
            break;
        case AmbientKind::RoomName: {
            void* a = acc::engine::GetCurrentArea();
            if (a) {
                // GetRoomDisplayName needs a char sink; room names are
                // short authored strings, so a local buffer is fine here.
                char roomTmp[160] = {0};
                acc::engine::GetRoomDisplayName(a, roomIdx,
                                                roomTmp, sizeof(roomTmp));
                return std::string(roomTmp);
            }
            break;
        }
        case AmbientKind::TerrainShape:
            if (shapeText && shapeText[0] != '\0') {
                return std::string(shapeText);
            }
            break;
        case AmbientKind::None:
            break;
    }
    return std::string();
}

}  // namespace acc::map_ui_cursor
