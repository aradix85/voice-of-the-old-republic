// Stepping through the cycle and announcing the focused entry: its name, payload and position, and the category when it changes.

#include "cycle_input_internal.h"

#include "cycle_state.h"
#include "engine_area.h"
#include "exit_memory.h"     // TargetWayInfo — where a door leads, the way there
#include "log.h"
#include "map_shipped_hints.h"  // shipped curated hints in the Map cycle
#include "map_ui_cursor.h"
#include "narrated_target.h"
#include "same_name_suffix.h"
#include "strings.h"
#include "view_mode.h"      // GetEffectiveViewpoint — the frame the
                            // clock + metres are measured from
#include "speech.h"

namespace acc::cycle_input {

namespace {

// Resolve a map pin's spoken note text exactly as AnnounceCurrent does,
// so the same-name comparison used for numbering keys off the string the
// user actually hears (note_text, or the "no text" fallback).
void ResolvePinNoteText(void* pin, char* outBuf, size_t bufSize) {
    if (!acc::engine::GetMapPinNoteText(pin, outBuf, bufSize) ||
        outBuf[0] == '\0') {
        std::snprintf(outBuf, bufSize, "%s",
                      acc::strings::Get(acc::strings::Id::MapPinNoText));
    }
}

// Resolve listing entry `idx`'s spoken label, mirroring AnnounceCurrent's
// focused-entry cascade so the same-name comparison used for numbering
// keys off exactly the string the user hears. Returns an empty key when no
// localized name resolves (the spoken text falls back to the category name,
// which carries no per-entry identity to number by).
void ResolveEntryName(const acc::cycle::CategoryListing& listing, int idx,
                      bool mapHint, char* out, size_t size) {
    out[0] = '\0';
    if (listing.isStatic[idx]) {
        acc::map_shipped_hints::GetLabel(listing.objs[idx], out, size);
        return;
    }
    if (listing.isPin[idx]) {
        ResolvePinNoteText(listing.objs[idx], out, size);
        return;
    }
    if (mapHint &&
        acc::engine::GetWaypointMapNote(listing.objs[idx], out, size) &&
        out[0] != '\0') {
        return;
    }
    // Base name — this is a same-name GROUPING key, so it must not carry a
    // door's live state (see AppendAreaPositionOrdinal). Identical to
    // GetObjectName for every kind this map-context path actually sees.
    if (!acc::engine::GetObjectBaseName(listing.objs[idx], out, size) ||
        out[0] == '\0') {
        out[0] = '\0';
    }
}

// Append a north-to-south positional ordinal to a LISTING entry's name when
// two or more entries in the listing share the same spoken label. Designer
// map "hints" (north path ×4) and duplicate placeables (footlocker ×3) repeat
// the same label; the appended number lets the user refer back to a specific
// one.
//
// Listing-relative (ranks within `listing`, not the whole area) — used only
// for the MAP context, where the listing is the full eligible pin/landmark
// set (never discovery-filtered), so listing-relative already equals global.
// It also handles map pins + map-note waypoints, whose spoken label comes
// from pin note_text / GetWaypointMapNote rather than GetObjectName, so the
// area-scanning AppendAreaPositionOrdinal can't substitute here.
//
// World-context statics instead go through acc::narration::AppendDisambiguator
// → AppendAreaPositionOrdinal, which ranks against the WHOLE area (not the
// discovery-filtered listing) so a number can't shift as more peers are
// discovered, and matches the number the Q/E focus path speaks for the same
// object.
void AppendPositionOrdinal(const acc::cycle::CategoryListing& listing,
                           int focusedIndex, bool mapHint, char* name,
                           size_t nameSize) {
    char focusKey[128];
    ResolveEntryName(listing, focusedIndex, mapHint, focusKey,
                     sizeof(focusKey));
    if (focusKey[0] == '\0') return;  // no per-entry label to key on

    // Count how many like-shaped, same-named peers sort north of the
    // focused entry; that rank (1-based, northmost = 1) is its ordinal.
    const bool    focusIsPin    = listing.isPin[focusedIndex];
    const bool    focusIsStatic = listing.isStatic[focusedIndex];
    const Vector& fp            = listing.positions[focusedIndex];
    int rank      = 1;
    int peerCount = 0;
    for (int j = 0; j < listing.count; ++j) {
        // Compare like with like — same entry shape only.
        if (listing.isPin[j] != focusIsPin) continue;
        if (listing.isStatic[j] != focusIsStatic) continue;
        char other[128];
        ResolveEntryName(listing, j, mapHint, other, sizeof(other));
        if (std::strcmp(other, focusKey) != 0) continue;
        ++peerCount;
        if (j != focusedIndex &&
            acc::narration::PositionLess(listing.positions[j], fp)) {
            ++rank;
        }
    }
    if (peerCount < 2) return;  // unique label — no number needed

    size_t curLen = std::strlen(name);
    if (curLen + 5 < nameSize) {
        std::snprintf(name + curLen, nameSize - curLen, " %d", rank);
    }
}

// Speak the locked Pillar 4 payload for whatever the cycle state
// currently focuses. `categoryPrefix` non-null wraps the item with
// "{category}. {item}" via the FmtCategoryItem template; null speaks
// just the item. Distance rounds to whole metres; sub-1m floors to 1
// (avoids "0 metres"). When player yaw is unavailable (degenerate
// facing during spawn / mid-load), the clock segment is dropped.
void AnnounceCurrent(const acc::cycle::CategoryListing& listing,
                     const char* categoryPrefix,
                     acc::filter::CycleContext ctx) {
    auto& s = acc::cycle::GetState(ctx);
    auto bindings = BindingsFor(s.category, ctx);
    const bool mapCtx = (ctx == acc::filter::CycleContext::Map);

    if (!s.focusedObj || s.focusedIndex < 0 ||
        s.focusedIndex >= listing.count) {
        // Empty state — speak the localized "no X in range" phrase. The
        // category-prefix wrapper is intentionally NOT applied here:
        // EmptyDoors etc. are full sentences, not item names.
        const char* msg = acc::strings::Get(bindings.empty);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "-> [%s] (ctx=%s)", msg,
                      mapCtx ? "Map" : "World");
        return;
    }

    const Vector& objPos = listing.positions[s.focusedIndex];

    char name[128] = "";
    // Four name-resolution paths share the "Map hint" announce:
    //  1) Shipped curated hint (listing.isStatic=true): the mod-localised
    //     label from the map_shipped_hints table.
    //  2) User-placed map pin (listing.isPin=true): CSWCMapPin.note_text
    //     (+0x100), populated by map_user_markers::BuildAutoName.
    //  3) Map-context waypoint: GetWaypointMapNote (+0x230) — the engine's
    //     localised map-note text, same string sighted players see.
    //  4) Anything else: GetObjectName (door / NPC / ...). World-context
    //     Landmark + Transition go here too — GetObjectName now folds the
    //     waypoint map-note (+0x230) and the trigger transition_destination
    //     (+0x30c) into its own fallback chain, so those resolve to the
    //     human-readable label ("To Upper City") instead of the raw tag.
    // The map-context branch below still reads GetWaypointMapNote directly
    // so its same-name numbering keys off the exact map string.
    bool isPin    = listing.isPin[s.focusedIndex];
    bool isStatic = listing.isStatic[s.focusedIndex];
    bool mapHint = mapCtx &&
                   s.category == acc::filter::CycleCategory::Landmark;
    bool nameOk = false;
    if (isStatic) {
        nameOk = acc::map_shipped_hints::GetLabel(s.focusedObj,
                                                  name, sizeof(name));
    } else if (isPin) {
        nameOk = acc::engine::GetMapPinNoteText(s.focusedObj,
                                                name, sizeof(name)) &&
                 name[0] != '\0';
        if (!nameOk) {
            std::snprintf(name, sizeof(name), "%s",
                          acc::strings::Get(acc::strings::Id::MapPinNoText));
            nameOk = true;
        }
    } else if (mapHint) {
        nameOk = acc::engine::GetWaypointMapNote(s.focusedObj,
                                                 name, sizeof(name)) &&
                 name[0] != '\0';
    }
    // Static hints never fall through to GetObjectName — the table row is
    // not an engine object; a failed GetLabel lands on the category name.
    if (!nameOk && !isStatic) {
        // Base name only; the door state suffix goes on after the ordinal
        // below, so the number lands right after the name.
        nameOk = acc::engine::GetObjectBaseName(s.focusedObj,
                                                name, sizeof(name)) &&
                 name[0] != '\0';
    }
    if (!nameOk) {
        // Fall back to the localized category name — at least the user
        // knows the kind even if no localized name resolves.
        std::snprintf(name, sizeof(name), "%s",
                      acc::strings::Get(bindings.name));
    }

    // Disambiguate same-name entries with a stable ordinal, so repeated
    // labels ("north path" ×4, "Kath Hound" ×3) are individually referable.
    // Two keying strategies by mobility, and they must match what the Q/E
    // focus path speaks for the same object:
    //  - Static objects number by north-to-south world position, so
    //    "north path 3" is the same marker every visit, save, and player — a
    //    global reference frame independent of entry direction or distance.
    //  - Creatures move; position-ranking would swap their numbers as they
    //    walk. They keep the handle-keyed same-name suffix instead, shared
    //    with combat/passive narration so a creature keeps one number
    //    everywhere.
    // Map context uses the listing-relative AppendPositionOrdinal: its
    // listing is the full eligible set (never discovery-filtered), and it
    // alone resolves pin / map-note labels. World context funnels through
    // acc::narration::AppendObjectQualifiers — the SAME entry point Q/E uses
    // via GetSpokenName — whose ordinal step routes creatures to the serial
    // and statics to the whole-area position ordinal (stable as more peers
    // are discovered).
    if (mapCtx) {
        AppendPositionOrdinal(listing, s.focusedIndex, mapHint, name,
                              sizeof(name));
    } else {
        // The full qualifier tail in one call — ordinal, door state
        // (", locked", ", To Bridge"), placeable state tag
        // (", trapped"), and the looted-container note (", empty"). Shared
        // verbatim with the Q/E path's GetSpokenName so an object speaks the
        // same string on both surfaces; this used to be three inlined calls
        // that left the empty-container note off the cycle only.
        acc::narration::AppendObjectQualifiers(s.focusedObj, s.category, name,
                                               sizeof(name));
    }

    Vector viewPos{};
    float yaw = 0.0f;
    bool haveYaw = acc::view_mode::GetEffectiveViewpoint(viewPos, yaw);
    int clock = 12;
    if (haveYaw) {
        clock = ClockPosition(yaw, objPos.x - viewPos.x,
                              objPos.y - viewPos.y);
    }
    int metres = static_cast<int>(listing.distances[s.focusedIndex] + 0.5f);

    char itemMsg[576];
    const int itemLen =
        FormatItemPayload(name, haveYaw, clock, metres, itemMsg, sizeof(itemMsg));
    // World objects: where a door leads, and the way to anything in another
    // region (the map cycle's pins and hints have no place in the graph).
    if (!mapCtx && !isPin && !isStatic && itemLen > 0 &&
        itemLen < static_cast<int>(sizeof(itemMsg))) {
        const std::string way =
            acc::exit_memory::TargetWayInfo(s.focusedObj, objPos);
        if (!way.empty()) {
            std::snprintf(itemMsg + itemLen, sizeof(itemMsg) - itemLen, ", %s",
                          way.c_str());
        }
    }

    char fullMsg[704];
    if (categoryPrefix && categoryPrefix[0] != '\0') {
        std::snprintf(fullMsg, sizeof(fullMsg),
                      acc::strings::Get(acc::strings::Id::FmtCategoryItem),
                      categoryPrefix, itemMsg);
    } else {
        std::snprintf(fullMsg, sizeof(fullMsg), "%s", itemMsg);
    }
    acc::speech::Speak(fullMsg, /*interrupt=*/true);

    // Stamp the unified activation slot — the user just heard the
    // object's name (with distance + clock), so Enter / Shift+- / Ctrl+- /
    // `-` should target it. Pin stamp shape carries the frozen position
    // since pins don't resolve through ResolveServerObjectHandle; game-
    // object stamp re-derives the server handle for the AI primitives.
    uint32_t serverHandle = 0;
    if (isPin || isStatic) {
        // Shipped hints share the map-pin stamp shape: frozen position,
        // no server handle. narrated_target validates them back through
        // map_shipped_hints::IsShippedHint.
        acc::narrated_target::StampMapPin(s.focusedObj, objPos);
    } else {
        serverHandle = acc::engine::GetObjectHandle(s.focusedObj);
        acc::narrated_target::Stamp(s.focusedObj, serverHandle);
    }

    // Map-context: pan the virtual cursor to the focused object so the
    // cursor stays coherent with the cycle (otherwise the cursor would
    // sit at its last WASD position, contradicting the spoken focus).
    // The cursor suppresses its own re-announce when handed a waypoint
    // we just spoke, so no duplicate output. Pins aren't waypoints —
    // pass nullptr so the cursor's waypoint-suppress doesn't latch on
    // an unrelated pointer.
    if (mapCtx) {
        acc::map_ui_cursor::PanToWorld(
            objPos,
            (!isPin && !isStatic &&
             s.category == acc::filter::CycleCategory::Landmark)
                ? s.focusedObj
                : nullptr);
    }

    acclog::Write("Cycle",
                  "-> [%s] obj=%p handle=0x%08x ctx=%s isPin=%d isStatic=%d",
                  fullMsg, s.focusedObj, serverHandle,
                  mapCtx ? "Map" : "World", (int)isPin, (int)isStatic);
}

}  // namespace

// Format the "{name}, {clock} o'clock, {metres} metres" payload (or the
// no-clock variant) into outBuf. Returns the bytes written (excluding
// terminator). Bound to acc::strings, so wording / locale lives there.
int FormatItemPayload(const char* name, bool haveYaw, int clock,
                      int metres, char* outBuf, size_t outBufSize) {
    if (metres < 1) metres = 1;
    const char* fmt = acc::strings::Get(haveYaw
        ? acc::strings::Id::FmtAnnounceWithClock
        : acc::strings::Id::FmtAnnounceNoClock);
    int n = haveYaw
        ? std::snprintf(outBuf, outBufSize, fmt, name, clock, metres)
        : std::snprintf(outBuf, outBufSize, fmt, name, metres);
    return n < 0 ? 0 : n;
}

// ---- Per-action handlers (shared by both ingestion paths) ----

// Step within the current category and announce the new focused item.
void OnCycleItem(bool prev, acc::filter::CycleContext ctx) {
    acc::cycle::CategoryListing listing;
    acc::cycle::RefreshCurrentListing(listing, ctx);
    if (prev) acc::cycle::CyclePrevItem(listing, ctx);
    else      acc::cycle::CycleNextItem(listing, ctx);
    AnnounceCurrent(listing, /*categoryPrefix=*/nullptr, ctx);
}

// Jump to the first (closest) or last (farthest) item of the current
// category and announce it. Same listing-refresh + announce flow as
// OnCycleItem; only the index target differs (Ctrl+, / Ctrl+.).
void OnCycleEdge(bool last, acc::filter::CycleContext ctx) {
    acc::cycle::CategoryListing listing;
    acc::cycle::RefreshCurrentListing(listing, ctx);
    if (last) acc::cycle::CycleLastItem(listing, ctx);
    else      acc::cycle::CycleFirstItem(listing, ctx);
    AnnounceCurrent(listing, /*categoryPrefix=*/nullptr, ctx);
}

// Step to the next/prev non-empty category and announce
// "{Category}. {closest item name}, {clock}, {metres}". When all six
// categories are empty, speaks the EmptyAll string without a prefix
// (the prefix would be misleading — no category was actually landed on).
void OnCycleCategory(bool prev, acc::filter::CycleContext ctx) {
    acc::cycle::CategoryListing listing;
    bool found = prev
        ? acc::cycle::CyclePrevCategory(listing, ctx)
        : acc::cycle::CycleNextCategory(listing, ctx);
    if (!found) {
        const char* msg = acc::strings::Get(acc::strings::Id::EmptyAll);
        acc::speech::Speak(msg, /*interrupt=*/true);
        acclog::Write("Cycle", "-> [%s] (all empty, ctx=%s)", msg,
                      ctx == acc::filter::CycleContext::Map ? "Map" : "World");
        return;
    }
    auto& s = acc::cycle::GetState(ctx);
    auto bindings = BindingsFor(s.category, ctx);
    AnnounceCurrent(listing, acc::strings::Get(bindings.name), ctx);
}

}  // namespace acc::cycle_input
