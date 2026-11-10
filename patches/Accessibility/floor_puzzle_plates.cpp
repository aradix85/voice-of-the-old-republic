// Floor-plate puzzle board model — see floor_puzzle_plates.h.

#include "floor_puzzle_plates.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace acc::floor_puzzle {

namespace {

// Signed distance from v to [lo, hi]: positive when the span lies above.
float ToSpan(float v, float lo, float hi) {
    if (v < lo) return lo - v;
    if (v > hi) return hi - v;
    return 0.0f;
}

}  // namespace

void ComputeCenterAndSpan(Plate& p) {
    if (p.vertCount < 1) return;
    p.center = {0.0f, 0.0f, 0.0f};
    p.minX = p.maxX = p.poly[0].x;
    p.minY = p.maxY = p.poly[0].y;
    for (int i = 0; i < p.vertCount; ++i) {
        const Vector& v = p.poly[i];
        p.center.x += v.x;
        p.center.y += v.y;
        p.center.z += v.z;
        p.minX = std::min(p.minX, v.x);
        p.maxX = std::max(p.maxX, v.x);
        p.minY = std::min(p.minY, v.y);
        p.maxY = std::max(p.maxY, v.y);
    }
    p.center.x /= p.vertCount;
    p.center.y /= p.vertCount;
    p.center.z /= p.vertCount;
}

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
    const Vector pt{x, y, 0.0f};
    float bestSq = DistXYToSegmentSq(pt, p.poly[p.vertCount - 1], p.poly[0]);
    for (int i = 1; i < p.vertCount; ++i) {
        bestSq = std::min(bestSq, DistXYToSegmentSq(pt, p.poly[i - 1], p.poly[i]));
    }
    return std::sqrt(bestSq);
}

SpanOffset OffsetToSpan(const Plate& p, const Vector& pos) {
    return SpanOffset{ToSpan(pos.y, p.minY, p.maxY), ToSpan(pos.x, p.minX, p.maxX)};
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
