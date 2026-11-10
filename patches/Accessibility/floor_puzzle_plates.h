// Engine-free model of the floor-plate puzzle board: the plate slots, their
// trigger polygons and the point-versus-plate geometry floor_puzzle.cpp polls.

#pragma once

#include <cstdint>

#include "engine_offsets_types.h"   // Vector

namespace acc::floor_puzzle {

constexpr int kPlateCount = 9;
constexpr int kResetIndex = 9;   // g_plates slot for the reset trigger
constexpr int kTrackCount = 10;
constexpr int kMaxPolyVerts = 8;

struct Plate {
    uint32_t trigHandle = 0;
    uint32_t plcHandle  = 0;   // FloorPanelNN placeable (lit-state carrier)
    Vector   poly[kMaxPolyVerts];
    int      vertCount  = 0;
    Vector   center     = {0.0f, 0.0f, 0.0f};
    // Axis-aligned span (the plates are authored as axis-aligned quads).
    // Directions speak the plate as an AREA, not a point: an axis counts
    // only when the player is outside this span on it.
    float    minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
};

bool PointInPoly(const Plate& p, float x, float y);

// 0 when inside; else the closest XY distance to any polygon edge.
float DistToPlate(const Plate& p, float x, float y);

// Trigger tag -> slot. Tags authored as kFloorPanel01..09 / kPanelReset;
// compare case-insensitively.
int TrigSlotForTag(const char* tag);

int PlcSlotForTag(const char* tag);

}  // namespace acc::floor_puzzle
