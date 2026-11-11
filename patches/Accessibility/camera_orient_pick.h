// Engine-free choice of where N turns the camera: the angle helpers, the
// exit to face among the region's exits, and the next cardinal when there
// are none. Angles are in the engine frame (East = 0, counter-clockwise
// positive) unless named compass (North = 0, clockwise).

#pragma once

#include <vector>

#include "engine_offsets_types.h"

namespace acc::camera_orient {

constexpr float kTwoPi = 6.28318530717958647692f;

// Within this of a heading counts as facing it (about 2.9 degrees).
constexpr float kArrivalRad = 0.05f;

// To (-pi, pi].
inline float NormaliseRad(float r) {
    while (r >  kPi) r -= kTwoPi;
    while (r <= -kPi) r += kTwoPi;
    return r;
}

// One exit of the player's region: its bearing from the player and its point.
struct ExitBearing {
    float  yawRad;
    Vector pos;
};

// Adds `e` unless a kept exit lies within kArrivalRad of its bearing: a turn
// between the two would do nothing.
void AddExitBearing(std::vector<ExitBearing>& exits, const ExitBearing& e);

// The next exit clockwise from exits[fromIdx]; fromIdx itself only when it is
// the only one.
int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx);

// The exit to turn to. lastIdx is the exit the previous press turned to, when
// the player is still in its region (else -1): still facing it, the press
// cycles on clockwise. Otherwise the exit nearest the heading, however small
// the turn, and when that one is already faced, the next clockwise. outWhy
// names the rule, for the log.
int ChooseExit(const std::vector<ExitBearing>& exits, int lastIdx, float curYawRad,
               const char*& outWhy);

// The next cardinal clockwise (N, E, S, W) after compassDeg.
float NextCardinalCompassDeg(float compassDeg);

float CompassDegToEngineRad(float compassDeg);

}  // namespace acc::camera_orient
