// Engine-free choice of where N turns the camera (camera_orient_pick.h).

#include "camera_orient_pick.h"

#include <cmath>

namespace acc::camera_orient {

namespace {

// "Still facing the exit the last press turned to": the turn lands a few
// degrees short or long, so exact equality never holds.
constexpr float kStillFacingRad = 10.0f * kDegToRad;

float AbsOffset(float yawRad, float curYawRad) {
    return std::fabs(NormaliseRad(yawRad - curYawRad));
}

}  // namespace

void AddExitBearing(std::vector<ExitBearing>& exits, const ExitBearing& e) {
    for (const ExitBearing& kept : exits) {
        if (AbsOffset(e.yawRad, kept.yawRad) <= kArrivalRad) return;
    }
    exits.push_back(e);
}

int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx) {
    int best = fromIdx;
    float bestCw = kTwoPi + 1.0f;
    for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
        if (i == fromIdx) continue;
        // Clockwise is decreasing yaw in the engine frame.
        const float cw =
            std::fmod(exits[fromIdx].yawRad - exits[i].yawRad + 2.0f * kTwoPi, kTwoPi);
        if (cw < bestCw) {
            bestCw = cw;
            best = i;
        }
    }
    return best;
}

int ChooseExit(const std::vector<ExitBearing>& exits, int lastIdx, float curYawRad,
               const char*& outWhy) {
    if (lastIdx >= 0 && AbsOffset(exits[lastIdx].yawRad, curYawRad) <= kStillFacingRad) {
        outWhy = "cycle";
        return NextExitClockwise(exits, lastIdx);
    }
    int nearest = 0;
    float nearestAbs = kTwoPi;
    for (int i = 0; i < static_cast<int>(exits.size()); ++i) {
        const float a = AbsOffset(exits[i].yawRad, curYawRad);
        if (a < nearestAbs) {
            nearestAbs = a;
            nearest = i;
        }
    }
    if (nearestAbs <= kArrivalRad) {
        outWhy = "already-facing";
        return NextExitClockwise(exits, nearest);
    }
    outWhy = "nearest";
    return nearest;
}

float NextCardinalCompassDeg(float compassDeg) {
    float deg = std::fmod(compassDeg, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return static_cast<float>((static_cast<int>(deg / 90.0f) + 1) % 4) * 90.0f;
}

float CompassDegToEngineRad(float compassDeg) {
    float engineDeg = std::fmod(90.0f - compassDeg + 360.0f, 360.0f);
    if (engineDeg < 0.0f) engineDeg += 360.0f;
    return engineDeg * kDegToRad;
}

}  // namespace acc::camera_orient
