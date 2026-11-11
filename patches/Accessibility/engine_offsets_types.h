// Engine C++ type definitions shared across the patch.
//
// The engine_offsets.h family is split by what decides portability between
// the two games:
//
//   engine_offsets_types.h      This file. Plain C++ struct layouts for engine
//                               types we read through or construct, and the
//                               distances between positions. No addresses,
//                               no offsets.
//   engine_offsets_addresses.h  Executable addresses: functions, vtables and
//                               .data global pointers, through acc::addr.
//   engine_offsets_fields.h     Struct field offsets.
//   engine_offsets_values.h     Constants that are neither: vtable slot
//                               indices, TLK strrefs, sentinels, enum bytes,
//                               panel input codes.
//
// engine_offsets.h includes all four.
//
// On KOTOR 2 each group moves on its own: every address differs, struct
// offsets mostly differ, and resource-derived values follow dialog.tlk and
// the .gui files, not the executable.

#pragma once

#include <cmath>
#include <cstdint>

// CExoArrayList<T*> — engine container for arrays of pointers.
struct CExoArrayList {
    void** data;
    int    size;
    int    capacity;
};

// Vector — Aurora-engine 3D vector. Used for world position, object
// orientation (heading vector with z=0), audio listener pose, etc.
//
// Coordinate frame (investigation Q1 — right-handed, Z-up):
//   +X = east, +Y = north, +Z = up. 1.0 unit ≈ 1 metre.
//   Bearing convention for object orientation: 0° = +X = east; CCW positive.
struct Vector {
    float x;
    float y;
    float z;
};

// Distances in metres. XY: on the floor plan, height ignored (what the player
// is told, walking distances, matching spots). XYZ: in space (audio falloff).
inline float DistXYSq(const Vector& a, const Vector& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

inline float DistXY(const Vector& a, const Vector& b) {
    return std::sqrt(DistXYSq(a, b));
}

// From p to the nearest point of the segment a-b, on the floor plan.
inline float DistXYToSegmentSq(const Vector& p, const Vector& a, const Vector& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }
    return DistXYSq(p, Vector{a.x + t * dx, a.y + t * dy, 0.0f});
}

inline float DistXYZSq(const Vector& a, const Vector& b) {
    const float dz = a.z - b.z;
    return DistXYSq(a, b) + dz * dz;
}

// A distance as the whole metres the player is told.
inline int WholeMetres(float metres) {
    return static_cast<int>(metres + 0.5f);
}

// The same for something that is not where the player stands: never 0.
inline int WholeMetresAtLeastOne(float metres) {
    const int whole = WholeMetres(metres);
    return whole < 1 ? 1 : whole;
}

// CExoString - the engine's string type: heap c_string plus explicit length.
// Most engine string accessors construct one in place into caller-supplied
// storage; see the CTlkTable / CSWFeat / CSWSItem entries in
// engine_offsets_addresses.h for the ownership rules that go with that.
struct CExoString {
    char*    c_string;
    uint32_t length;
};
