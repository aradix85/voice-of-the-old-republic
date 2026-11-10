// What is under the map cursor: the nearest explored map note, user marker
// or shipped hint within the hover radius.

#include "map_ui_cursor_internal.h"

#include "engine_area.h"
#include "engine_offsets_select.h"
#include "engine_player.h"
#include "map_shipped_hints.h"
#include "map_user_markers.h"

namespace acc::map_ui_cursor {

namespace {

// The hover radius in map pixels, squared: a 16 px pin icon plus slack.
// 24 px was hard to hit without sight; notes are sparse enough that 36 px
// rarely covers two, and the nearest wins anyway.
constexpr float kHoverHitRadius2 = 36.0f * 36.0f;

// CSWGuiInGameMap.map_hider, from the struct layout in swkotor.exe.h
// (CSWGuiInGameMap line 8548): after the panel, 4 labels and 5 buttons —
//   exit_button       @ +0x8ec  (size 0x1c4 → ends 0xab0)
//   up_button         @ +0xab0  (size 0x1c4 → ends 0xc74)
//   down_button       @ +0xc74  (size 0x1c4 → ends 0xe38)
//   map_hider         @ +0xe38  (CSWGuiMapHider, size 0x248)
// — and field11_0x238 inside the hider holds the waypoint linked list.
// KOTOR 2: hider embed at +0x1160, witnessed by its map-panel ctor
// 0x00893950 (`add ecx,0x1160` before the MapHider ctor 0x00895620; the
// same ctor's BTN_UP/BTN_DOWN wiring reproduces the already-witnessed
// +0x610/+0x7e0 button offsets). Waypoint list at +0x248 (K1 +0x238 — the
// hider grew +0x10), witnessed in the K2 GetNextMapNote twin 0x00896D10:
// its node walk calls the CExoLinkedList object at [hider+0x248] and reads
// each node's waypoint handle with the same 0x7f000000 sentinel fallback.
//
// The list's payload per node is the waypoint's OBJECT HANDLE, which the
// engine resolves through CServerExoApp::GetGameObject, as for
// CSWSArea.game_objects. A waypoint's position is CSWSObject.Position
// (kServerObjectPositionOffset); on KOTOR 2 that is derived, not yet seen
// on a live waypoint read.
const size_t kInGameMapHiderOffset       = acc::off::Pick(0xe38, 0x1160);
const size_t kMapHiderWaypointListOffset = acc::off::Pick(0x238, 0x248);  // CExoLinkedList

// Takes `obj` as the best candidate when its map pixel is nearer the cursor
// than the best so far, which starts at the hover radius.
void Consider(Nearest& best, void* obj, float ppx, float ppy,
              float cursorPx, float cursorPy) {
    const float dx = ppx - cursorPx;
    const float dy = ppy - cursorPy;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best.dist2) {
        best.dist2 = d2;
        best.obj   = obj;
    }
}

}  // namespace

// The nearest explored map note: the map hider's waypoint list under the
// filter the engine's GetNext/PrevMapNote applies (map note enabled,
// position explored), but by distance rather than list order. `dump`
// collects one line per waypoint; the caller owns it because this function
// uses __try, and MSVC's C2712 forbids an object needing unwinding here. The
// lines cycle (#0, #1, ..., #0), so only a block log can fold a still map's
// repeats.
Nearest FindNearestExploredMapNote(void* mapPanel, void* areaMap,
                                   float cursorPx, float cursorPy,
                                   acclog::BlockLog& dump) {
    Nearest best{nullptr, kHoverHitRadius2};
    if (!mapPanel || !areaMap) return best;

    void* node = nullptr;
    __try {
        auto* list = reinterpret_cast<unsigned char*>(mapPanel) +
                     kInGameMapHiderOffset + kMapHiderWaypointListOffset;
        void* internal = *reinterpret_cast<void**>(list + kListInternalOffset);
        if (internal) {
            node = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(internal) + kListInternalHeadOffset);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return best;
    }

    for (int guard = 0; node && guard < 256; ++guard) {
        ++best.scanned;
        // CExoLinkedList<ulong>: each node's data is a heap-boxed handle
        // (GetNextMapNote: GetAtPos returns ulong*, the caller derefs it).
        uint32_t handle = 0;
        void* next = nullptr;
        __try {
            auto* n = reinterpret_cast<unsigned char*>(node);
            void* data = *reinterpret_cast<void**>(n + kLinkedListNodeDataOff);
            if (data) handle = *reinterpret_cast<uint32_t*>(data);
            next = *reinterpret_cast<void**>(n + kLinkedListNodeNextOff);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            break;
        }
        node = next;
        if (handle == 0) continue;

        void* obj = acc::engine::ResolveServerObjectHandle(handle);
        if (!obj) continue;

        int   enabled = 0;
        Vector pos    = {0, 0, 0};
        __try {
            auto* o = reinterpret_cast<unsigned char*>(obj);
            enabled = *reinterpret_cast<int*>(o + kWaypointMapNoteEnabledOffset);
            pos     = *reinterpret_cast<Vector*>(o + kServerObjectPositionOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }
        if (enabled == 0) continue;
        if (!acc::engine::IsWorldPointExplored(areaMap, pos)) continue;

        float ppx, ppy;
        if (!WorldToPixel(areaMap, pos, ppx, ppy)) continue;
        dump.Line("waypoint #%d handle=0x%x world=(%.2f,%.2f) pixel=(%.1f,%.1f)",
                  best.scanned, handle, pos.x, pos.y, ppx, ppy);
        Consider(best, obj, ppx, ppy, cursorPx, cursorPy);
    }
    return best;
}

// The nearest of the player's own markers in CSWCArea.map_pins[].
// Identified through the map_user_markers registry, not a flag bit: engine
// map-note pins carry the waypoint's client object id (high bit set), so a
// bit test matched them too and leaked unexplored notes. Markers skip fog:
// the player placed them.
Nearest FindNearestUserMapPin(void* clientArea, void* areaMap,
                              float cursorPx, float cursorPy) {
    Nearest best{nullptr, kHoverHitRadius2};
    if (!clientArea || !areaMap) return best;
    const int pinCount = acc::engine::GetMapPinCount(clientArea);
    for (int i = 0; i < pinCount; ++i) {
        void* pin = acc::engine::GetMapPinAt(clientArea, i);
        if (!pin) continue;
        ++best.scanned;
        if (!acc::map_user_markers::IsUserMarkerPin(pin)) continue;
        if (!acc::engine::IsMapPinEnabled(pin)) continue;
        Vector pos;
        if (!acc::engine::GetMapPinPosition(pin, pos)) continue;
        float ppx, ppy;
        if (!WorldToPixel(areaMap, pos, ppx, ppy)) continue;
        Consider(best, pin, ppx, ppy, cursorPx, cursorPy);
    }
    return best;
}

// The nearest of the mod's shipped curated hints (map_shipped_hints: table
// rows, no engine object). Fog-gated like engine notes, as in the cycle, so
// both agree on what exists.
Nearest FindNearestShippedHint(void* areaMap, float cursorPx, float cursorPy) {
    Nearest best{nullptr, kHoverHitRadius2};
    if (!areaMap) return best;
    const acc::map_shipped_hints::ShippedHint* hints[8];
    best.scanned = acc::map_shipped_hints::CollectForCurrentModule(hints, 8);
    for (int i = 0; i < best.scanned; ++i) {
        if (!acc::engine::IsWorldPointExplored(areaMap, hints[i]->pos)) continue;
        float ppx, ppy;
        if (!WorldToPixel(areaMap, hints[i]->pos, ppx, ppy)) continue;
        Consider(best, const_cast<void*>(static_cast<const void*>(hints[i])),
                 ppx, ppy, cursorPx, cursorPy);
    }
    return best;
}

}  // namespace acc::map_ui_cursor
