// Internal to the map_ui_cursor module (map_ui_cursor*.cpp); no other file includes
// it. The public interface is map_ui_cursor.h. Here: what the files share.

#pragma once

#include "map_ui_cursor.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <string>

#include "log.h"
#include "map_shipped_hints.h"        // shipped curated hints in the hover scan

namespace acc::map_ui_cursor {

// GetAreaMap / IsWorldPointExplored entry points moved to engine_area.
// engine_area.h carries the shared constants (kAddrCServerExoAppGetModule,
// kModuleAreaMapOffset, kAddrCSWSAreaMapIsWorldPointExplored).
// Ambient categories the cursor can sit on when it is NOT directly over
// a discrete map-note waypoint. Mutually exclusive: cursor is either in
// fog of war (Unexplored), or in an explored layout-room that has a
// cached Bioware-authored landmark (Landmark — Tier 1), or in an
// explored layout-room whose CSWSArea.room_names[] entry is a mod-
// supplied human-readable label (RoomName — Tier 2, rare in vanilla),
// or none of the above (None — silent).
//
// Vanilla resref-style room ids (`m02_03e`, `stunt_01_main`, ...) never
// reach RoomName because IsResrefStyleRoomName filters them out — those
// rooms classify as None and the cursor stays quiet over them, by
// design. The cursor surface is "what's HERE" — silence over a chunk
// the engine couldn't name is honest.
enum class AmbientKind {
    None,
    Unexplored,
    Landmark,
    RoomName,
    // Terrain shape synthesised from a 4-direction wall probe. Falls
    // through when no waypoint/landmark/named-room match — gives blind
    // players a sense of corridor vs. room vs. junction the same way a
    // sighted player reads geometry off the minimap render.
    TerrainShape,
};

// Helpers shared across the files, by the file that defines them.

// map_ui_cursor_transform.cpp
bool WorldToPixel(void* areaMap, const Vector& world, float& outPx, float& outPy);
bool PixelToWorld(void* areaMap, float px, float py, float zSeed, Vector& outWorld);

// map_ui_cursor_hover.cpp
void* FindNearestExploredMapNote(void* mapPanel, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr, acclog::BlockLog* dump = nullptr);
void* FindNearestUserMapPin(void* clientArea, void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr);
const acc::map_shipped_hints::ShippedHint* FindNearestShippedHint( void* areaMap, float cursorPx, float cursorPy, int* outScannedCount = nullptr, float* outBestDist2 = nullptr);
bool ReadWaypointMapNoteText(void* waypoint, char* outBuf, size_t bufSize);
bool ReadWaypointTag(void* waypoint, char* outBuf, size_t bufSize);

// map_ui_cursor_ambient.cpp
const char* AmbientKindStr(AmbientKind k);
std::string ResolveAmbientText(AmbientKind kind, int roomIdx, const char* landmarkBuf, const char* shapeText);

}  // namespace acc::map_ui_cursor
