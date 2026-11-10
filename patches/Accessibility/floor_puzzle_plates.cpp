// Floor-plate puzzle board model — see floor_puzzle_plates.h.

#include "floor_puzzle_plates.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace acc::floor_puzzle {

namespace {

float DistToSegment(float px, float py, const Vector& a, const Vector& b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((px - a.x) * dx + (py - a.y) * dy) / len2;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }
    float cx = a.x + t * dx - px, cy = a.y + t * dy - py;
    return std::sqrt(cx * cx + cy * cy);
}

}  // namespace

bool PointInPoly(const Plate& p, float x, float y) {
    if (p.vertCount < 3) return false;
    bool in = false;
    for (int i = 0, j = p.vertCount - 1; i < p.vertCount; j = i++) {
        const Vector& a = p.poly[i];
        const Vector& b = p.poly[j];
        if ((a.y > y) != (b.y > y) &&
            x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x) {
            in = !in;
        }
    }
    return in;
}

float DistToPlate(const Plate& p, float x, float y) {
    if (p.vertCount < 3) return 1e9f;
    if (PointInPoly(p, x, y)) return 0.0f;
    float best = 1e9f;
    for (int i = 0, j = p.vertCount - 1; i < p.vertCount; j = i++) {
        float d = DistToSegment(x, y, p.poly[i], p.poly[j]);
        if (d < best) best = d;
    }
    return best;
}

int TrigSlotForTag(const char* tag) {
    if (_stricmp(tag, "kPanelReset") == 0) return kResetIndex;
    if (_strnicmp(tag, "kFloorPanel", 11) == 0) {
        int n = atoi(tag + 11);
        if (n >= 1 && n <= kPlateCount) return n - 1;
    }
    return -1;
}

int PlcSlotForTag(const char* tag) {
    if (_strnicmp(tag, "FloorPanel", 10) == 0) {
        int n = atoi(tag + 10);
        if (n >= 1 && n <= kPlateCount) return n - 1;
    }
    return -1;
}

}  // namespace acc::floor_puzzle
