// What is under the map cursor: the nearest explored map note, user marker or shipped hint within the hover radius, and the waypoint's note text and tag.

#include "map_ui_cursor_internal.h"

#include "engine_area.h"
#include "engine_reads.h"             // ReadCExoString — waypoint tag log
#include "map_user_markers.h"         // IsUserMarkerPin — fog-exempt marker id
#include "engine_offsets_select.h"
                                      // the in-world walking adapter speaks
                                      // from, so cursor + walking agree

namespace acc::map_ui_cursor {

namespace {

// Radius (in map pixels) within which a map-note waypoint counts as
// "under the cursor". A 16-px map-pin icon plus generous slack —
// landmarks on the in-game map are sparse enough that 36 px overlap
// between adjacent pins is rare even on landmark-dense maps, and the
// hover-pause debounce discriminates anyway (the closer-of-two scan in
// FindNearestExploredMapNote always picks the nearest pin to the
// cursor centre). Tested live: 24 px was hard to pinpoint without
// sighted feedback; 36 px lets the cursor catch landmarks reliably.
constexpr int kHoverHitRadiusPx = 36;

// CSWGuiInGameMap.map_hider field offset. Derived from the struct layout
// in swkotor.exe.h (CSWGuiInGameMap line 8548): after panel (CSWGuiPanel)
// + 4 labels + 5 buttons, the embedded CSWGuiMapHider sits at the offset
// the GoG xml confirms via the previous fields. The xml gives:
//   exit_button       @ +0x8ec  (size 0x1c4 → ends 0xab0)
//   up_button         @ +0xab0  (size 0x1c4 → ends 0xc74)
//   down_button       @ +0xc74  (size 0x1c4 → ends 0xe38)
//   map_hider         @ +0xe38  (CSWGuiMapHider, size 0x248)
// Then field11_0x238 inside map_hider holds the waypoint linked list.
// KOTOR 2: hider embed at +0x1160, witnessed by its map-panel ctor
// 0x00893950 (`add ecx,0x1160` before the MapHider ctor 0x00895620; the
// same ctor's BTN_UP/BTN_DOWN wiring reproduces the already-witnessed
// +0x610/+0x7e0 button offsets). Waypoint list at +0x248 (K1 +0x238 — the
// hider grew +0x10), witnessed in the K2 GetNextMapNote twin 0x00896D10:
// its node walk calls the CExoLinkedList object at [hider+0x248] and reads
// each node's waypoint handle with the same 0x7f000000 sentinel fallback.
const size_t kInGameMapHiderOffset                = acc::off::Pick(0xe38, 0x1160);
const size_t kMapHiderWaypointListOffset          = acc::off::Pick(0x238, 0x248);  // CExoLinkedList

// CExoLinkedList layout (visible in engine_area / from Ghidra): the
// list holds an internal pointer; the internal exposes head/tail and
// nodes that point at the payload. For the map-hider list, the payload
// at each node is a `ulong` — the waypoint OBJECT HANDLE — which the
// engine then resolves via CServerExoApp::GetGameObject. Same access
// shape as CSWSArea.game_objects (handle array, not object pointer
// array).
// CSWSWaypoint layout offsets (already used elsewhere — re-stated here
// so the cursor code is self-documenting).
// DERIVED, not database-verified: this is CSWSObject.Position read on a
// waypoint (CSWSWaypoint derives from CSWSObject, and the value is identical to
// kServerObjectPositionOffset), so it inherits that field's +4 KOTOR 2 shift.
// The seeded database lists CSWSObject.Position but says nothing about
// waypoints specifically. Confirm against a real K2 waypoint read before
// trusting map-cursor output there.
const size_t kWaypointPositionOffset = acc::off::Pick(0x90, 0x94);

}  // namespace

// Walk the map-hider's waypoint linked list, find the nearest waypoint
// whose map-pixel projection lands within kHoverHitRadiusPx of the
// cursor AND whose has_map_note != 0 AND whose position
// IsWorldPointExplored == true. Returns the CSWSWaypoint* (server-side
// object) of the best match, or nullptr.
//
// Same filter the engine's GetNext/PrevMapNote apply — we re-do the
// scan rather than reading their cached current-node so we get spatial
// not sequential hit detection.
// `dump` (optional) collects the per-waypoint diagnostic lines. It is a
// POINTER rather than a local BlockLog because this function uses __try, and
// MSVC's C2712 forbids an object needing unwinding in a function with SEH —
// the same constraint menus_extract.cpp documents. The caller owns the block,
// so the whole per-tick scan folds as one unit when nothing moved. Line-level
// dedup cannot fold this: the lines cycle (#0,#1,…,#6,#0,#1,…), so each line
// differs from the one just before it and Trace never sees a repeat.
void* FindNearestExploredMapNote(void* mapPanel, void* areaMap,
                                 float cursorPx, float cursorPy,
                                 int* outScannedCount,
                                 float* outBestDist2,
                                 acclog::BlockLog* dump) {
    if (!mapPanel || !areaMap) return nullptr;
    void* mapHider = reinterpret_cast<unsigned char*>(mapPanel) +
                     kInGameMapHiderOffset;
    void* internal = nullptr;
    __try {
        internal = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(mapHider) +
            kMapHiderWaypointListOffset +
            kListInternalOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    if (!internal) return nullptr;

    // serverApp gate removed when GetServerApp lifted to engine_area
    // (Phase 6 lay-off 1a). ResolveServerObjectHandle handles its own
    // chain bail-out on null AppManager / CServerExoApp internally.

    void* bestObj = nullptr;
    float bestDist2 = (float)(kHoverHitRadiusPx * kHoverHitRadiusPx);

    void* node = nullptr;
    __try {
        node = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internal) +
            kListInternalHeadOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }

    int guard = 0;
    int scanned = 0;
    while (node && guard++ < 256) {
        ++scanned;
        // CExoLinkedList<ulong>: each node's data is a heap-boxed ulong*,
        // so the handle requires two indirections from `node`. Decomp of
        // GetNextMapNote confirms: GetAtPos returns ulong* and the caller
        // does `uVar7 = *puVar2;` to extract the handle value.
        uint32_t handle = 0;
        void* nextNode = nullptr;
        __try {
            void* dataPtr = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) +
                kLinkedListNodeDataOff);
            if (dataPtr) {
                handle = *reinterpret_cast<uint32_t*>(dataPtr);
            }
            nextNode = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) +
                kLinkedListNodeNextOff);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            break;
        }
        if (handle == 0) { node = nextNode; continue; }

        void* obj = acc::engine::ResolveServerObjectHandle(handle);
        if (!obj) { node = nextNode; continue; }

        // Spoiler / curation gate — same checks the engine uses
        // internally for GetNext/PrevMapNote.
        int hasNote = 0;
        Vector pos = {0,0,0};
        __try {
            hasNote = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(obj) +
                kWaypointMapNoteEnabledOffset);
            pos = *reinterpret_cast<Vector*>(
                reinterpret_cast<unsigned char*>(obj) +
                kWaypointPositionOffset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            node = nextNode; continue;
        }
        if (hasNote == 0) { node = nextNode; continue; }
        if (!acc::engine::IsWorldPointExplored(areaMap, pos)) {
            node = nextNode; continue;
        }

        float ppx, ppy;
        if (!WorldToPixel(areaMap, pos, ppx, ppy)) {
            node = nextNode; continue;
        }
        if (dump) {
            dump->Line("waypoint #%d handle=0x%x world=(%.2f,%.2f) "
                       "pixel=(%.1f,%.1f)",
                       scanned, handle, pos.x, pos.y, ppx, ppy);
        }
        float dx = ppx - cursorPx;
        float dy = ppy - cursorPy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestDist2) {
            bestDist2 = d2;
            bestObj = obj;
        }
        node = nextNode;
    }
    if (outScannedCount) *outScannedCount = scanned;
    if (outBestDist2) *outBestDist2 = bestObj ? bestDist2 : 1e30f;
    return bestObj;
}

// Hit-test against the mod's own saved markers in `CSWCArea.map_pins[]`.
// Markers are identified by IDENTITY (map_user_markers registry), not by a
// reference-number bit test: engine map-note pins are keyed by the
// waypoint's client object id (always high-bit set), so the former
// `flags & 0x80000000` filter matched them too and leaked unexplored
// notes. Engine notes are handled solely by FindNearestExploredMapNote,
// which is fog-gated. Returns nullptr if no user marker is within hover
// radius. The closer-of-two scan in the hover loop picks waypoint or
// marker based on pixel distance.
void* FindNearestUserMapPin(void* clientArea, void* areaMap,
                            float cursorPx, float cursorPy,
                            int* outScannedCount,
                            float* outBestDist2) {
    if (outBestDist2) *outBestDist2 = 1e30f;
    if (!clientArea || !areaMap) {
        if (outScannedCount) *outScannedCount = 0;
        return nullptr;
    }
    int pinCount = acc::engine::GetMapPinCount(clientArea);
    void* bestPin = nullptr;
    float bestDist2 = (float)(kHoverHitRadiusPx * kHoverHitRadiusPx);
    int scanned = 0;
    for (int i = 0; i < pinCount; ++i) {
        void* pin = acc::engine::GetMapPinAt(clientArea, i);
        if (!pin) continue;
        ++scanned;
        if (!acc::map_user_markers::IsUserMarkerPin(pin)) continue;  // ours only
        if (!acc::engine::IsMapPinEnabled(pin)) continue;
        Vector pos;
        if (!acc::engine::GetMapPinPosition(pin, pos)) continue;
        // User markers skip fog: the player dropped them, so revealing their
        // own location to themselves isn't a spoiler.
        float ppx, ppy;
        if (!WorldToPixel(areaMap, pos, ppx, ppy)) continue;
        float dx = ppx - cursorPx;
        float dy = ppy - cursorPy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestDist2) {
            bestDist2 = d2;
            bestPin = pin;
        }
    }
    if (outScannedCount) *outScannedCount = scanned;
    if (outBestDist2) *outBestDist2 = bestPin ? bestDist2 : 1e30f;
    return bestPin;
}

// Hit-test the mod's shipped curated hints (map_shipped_hints) — static
// table rows with no engine object behind them. Fog-GATED like engine
// notes (unlike user markers): a hint only becomes hoverable once its
// cell is explored, mirroring the cycle's gate so both surfaces agree on
// what exists.
const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint(
        void* areaMap, float cursorPx, float cursorPy,
        int* outScannedCount, float* outBestDist2) {
    if (outScannedCount) *outScannedCount = 0;
    if (outBestDist2) *outBestDist2 = 1e30f;
    if (!areaMap) return nullptr;
    const acc::map_shipped_hints::ShippedHint* hints[8];
    int n = acc::map_shipped_hints::CollectForCurrentModule(hints, 8);
    const acc::map_shipped_hints::ShippedHint* best = nullptr;
    float bestDist2 = (float)(kHoverHitRadiusPx * kHoverHitRadiusPx);
    int scanned = 0;
    for (int i = 0; i < n; ++i) {
        ++scanned;
        if (!acc::engine::IsWorldPointExplored(areaMap, hints[i]->pos)) {
            continue;
        }
        float ppx, ppy;
        if (!WorldToPixel(areaMap, hints[i]->pos, ppx, ppy)) continue;
        float dx = ppx - cursorPx;
        float dy = ppy - cursorPy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestDist2) {
            bestDist2 = d2;
            best = hints[i];
        }
    }
    if (outScannedCount) *outScannedCount = scanned;
    if (outBestDist2) *outBestDist2 = best ? bestDist2 : 1e30f;
    return best;
}

bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize) {
    return acc::engine::GetWaypointMapNote(waypoint, outBuf, bufSize);
}

// Read CSWSWaypoint.Tag for diagnostic logging. CExoString at
// kObjectTagOffset (engine_area.h). Returns false if the read faults
// or the tag is empty.
bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize) {
    if (!waypoint || !outBuf || bufSize < 2) return false;
    __try {
        // kObjectTagOffset lives at global scope in engine_area.h between
        // two acc::engine namespace blocks — reference it directly, not
        // through the namespace.
        return acc::engine::ReadCExoString(
            waypoint, kObjectTagOffset, outBuf, bufSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace acc::map_ui_cursor
