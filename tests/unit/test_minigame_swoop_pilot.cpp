// The automatic pilot's line around the buoys of the KOTOR 1 swoop tracks
// (minigame_swoop_pilot).

#include <cmath>
#include <cstdint>

#include "doctest.h"
#include "minigame_swoop_pilot.h"
#include "swoop_tracks_k1.h"

using acc::swoop_pilot::Bike;
using acc::swoop_pilot::kPassLeft;
using acc::swoop_pilot::kPassRight;
using acc::swoop_pilot::kPilotStepU;
using acc::swoop_pilot::LineAroundRocks;
using acc::swoop_pilot::Rock;
using doctest::Approx;

namespace {

// A bike 10 units left of the middle of the lane, 5 units of travel per tick.
// With its radius of 3 it stays out of x 93.5 .. 106.5 beside kRock.
const Bike kBike{90.0f, 0.0f, 100.0f, 3.0f, 5.0f};
const Rock kRock{0, 98.0f, 102.0f, 30.0f, 38.0f};

float Line(const Bike& bike, float goalX, float goalY, const Rock& rock, int8_t side = 0,
           int* blocking = nullptr) {
    int8_t sides[1] = {side};
    int slot = -1;
    const float line = LineAroundRocks(bike, goalX, goalY, &rock, 1, sides, slot);
    if (blocking) *blocking = slot;
    return line;
}

float ClampTo(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

struct Outcome {
    int hits;
    int missed;
};

// One race at a steady speed: each tick the bike steps sideways toward the
// line, at most kPilotStepU, and then travels `fwd`. A buoy counts as hit when
// the bike's sphere overlaps its box, a pad as missed when it is crossed more
// than 6 units to the side (the two spheres of 3). `steerAround` off follows
// the pads alone.
Outcome Race(const swoop_tracks::Track& track, float fwd, bool steerAround) {
    int8_t sides[32] = {};
    bool   hit[32]   = {};
    Outcome out{0, 0};
    Bike bike{swoop_tracks::kLaneX, 0.0f, swoop_tracks::kLaneX, 3.0f, fwd};
    const float end = track.rocks[track.rockCount - 1].y1 + 100.0f;
    while (bike.y < end) {
        const swoop_tracks::Pad* pad = nullptr;
        for (int i = 0; i < track.padCount && !pad; ++i) {
            if (track.pads[i].y >= bike.y) pad = &track.pads[i];
        }
        const float goalX = pad ? pad->x : bike.x;
        const float goalY = pad ? pad->y : bike.y + 400.0f;
        int slot = -1;
        const float line = steerAround ? LineAroundRocks(bike, goalX, goalY, track.rocks,
                                                         track.rockCount, sides, slot)
                                       : goalX;
        bike.x += ClampTo(line - bike.x, -kPilotStepU, kPilotStepU);
        bike.x = ClampTo(bike.x, bike.laneX - 20.0f, bike.laneX + 20.0f);
        const float nextY = bike.y + fwd;
        for (int i = 0; i < track.rockCount; ++i) {
            const Rock& r = track.rocks[i];
            if (!hit[i] && r.x0 - bike.radius < bike.x && bike.x < r.x1 + bike.radius &&
                r.y0 - bike.radius < nextY && bike.y < r.y1 + bike.radius) {
                hit[i] = true;
                ++out.hits;
            }
        }
        for (int i = 0; i < track.padCount; ++i) {
            const swoop_tracks::Pad& p = track.pads[i];
            if (bike.y < p.y && p.y <= nextY && std::fabs(p.x - bike.x) > 6.0f) ++out.missed;
        }
        bike.y = nextY;
    }
    return out;
}

const float kSpeeds[] = {2.0f, 3.5f, 5.0f, 7.0f, 9.0f, 11.0f, 13.0f, 16.0f};

}  // namespace

TEST_CASE("Without a rock in the way the line is the pad") {
    int8_t sides[1] = {0};
    int slot = 7;
    CHECK(LineAroundRocks(kBike, 101.0f, 60.0f, nullptr, 0, sides, slot) == Approx(101.0f));
    CHECK(slot == -1);

    const Rock aside{0, 110.0f, 114.0f, 30.0f, 38.0f};
    int blocking = 7;
    CHECK(Line(kBike, 95.0f, 60.0f, aside, 0, &blocking) == Approx(95.0f));
    CHECK(blocking == -1);
}

TEST_CASE("A rock in the pad's lane is passed on the side nearest the pad") {
    int blocking = -1;
    CHECK(Line(kBike, 101.0f, 60.0f, kRock, 0, &blocking) == Approx(106.5f));
    CHECK(blocking == 0);
    CHECK(Line(kBike, 99.0f, 60.0f, kRock) == Approx(93.5f));
}

TEST_CASE("Too close to cross in front, the bike stays on its own side") {
    const Rock close{0, 98.0f, 102.0f, 15.0f, 23.0f};
    CHECK(Line(kBike, 101.0f, 60.0f, close) == Approx(93.5f));
    CHECK(Line(kBike, 115.0f, 60.0f, close) == Approx(93.5f));
}

TEST_CASE("With time to cross in front the line stays the pad") {
    int blocking = 7;
    CHECK(Line(kBike, 115.0f, 60.0f, kRock, 0, &blocking) == Approx(115.0f));
    CHECK(blocking == -1);
}

TEST_CASE("A rock far ahead is not decided yet") {
    const Rock far{0, 98.0f, 102.0f, 80.0f, 88.0f};
    CHECK(Line(kBike, 101.0f, 120.0f, far) == Approx(101.0f));
}

TEST_CASE("A rock behind the pad waits while there is time to get clear after the pad") {
    // The pad lies in the rock's strip, which closes at y 22. From a pad at
    // y 10 the bike has 2.4 ticks, 9.6 units sideways: enough to get clear.
    // From a pad at y 15 it has 5.6 units, short of the 6.5 from the middle.
    CHECK(Line(kBike, 101.0f, 10.0f, kRock) == Approx(101.0f));
    CHECK(Line(kBike, 100.0f, 15.0f, kRock) == Approx(93.5f));
}

TEST_CASE("A side beyond the edge of the lane is never taken") {
    // Staying clear on the left would need x 76.5; the lane ends at 80.
    const Rock edge{0, 81.0f, 86.0f, 15.0f, 23.0f};
    const Bike bike{82.0f, 0.0f, 100.0f, 3.0f, 5.0f};
    CHECK(Line(bike, 83.0f, 60.0f, edge) == Approx(90.5f));
}

TEST_CASE("A chosen side sticks until the rock is behind") {
    const Rock close{0, 98.0f, 102.0f, 15.0f, 23.0f};
    CHECK(Line(kBike, 101.0f, 60.0f, close, kPassRight) == Approx(106.5f));
    CHECK(Line(kBike, 115.0f, 60.0f, kRock, kPassLeft) == Approx(93.5f));

    const Bike alongside{90.0f, 26.0f, 100.0f, 3.0f, 5.0f};
    CHECK(Line(alongside, 101.0f, 60.0f, close, kPassLeft) == Approx(93.5f));
    const Bike past{90.0f, 27.0f, 100.0f, 3.0f, 5.0f};
    CHECK(Line(past, 101.0f, 60.0f, close, kPassLeft) == Approx(101.0f));
}

TEST_CASE("Of two rocks the nearer one decides") {
    // The far rock is to be passed on the right, beyond x 108.5. The near
    // rock's strip is x 101.5 .. 114.5 and too close to cross: stay left of it.
    const Rock rocks[] = {{0, 106.0f, 110.0f, 15.0f, 23.0f}, {1, 100.0f, 104.0f, 30.0f, 38.0f}};
    int8_t sides[2] = {0, kPassRight};
    int slot = -1;
    const Bike bike{100.0f, 0.0f, 100.0f, 3.0f, 5.0f};
    CHECK(LineAroundRocks(bike, 112.0f, 60.0f, rocks, 2, sides, slot) == Approx(101.5f));
    CHECK(slot == 0);
    CHECK(sides[0] == kPassLeft);
}

TEST_CASE("The pilot touches no buoy on the three KOTOR 1 tracks, at any speed") {
    for (const swoop_tracks::Track& track : swoop_tracks::kTracks) {
        for (const float fwd : kSpeeds) {
            CAPTURE(track.name);
            CAPTURE(fwd);
            const Outcome out = Race(track, fwd, /*steerAround=*/true);
            CHECK(out.hits == 0);
            // Up to 9 units per tick the detours cost at most the pads that
            // cannot be had anyway (Taris ends on two pads side by side).
            if (fwd <= 9.0f) CHECK(out.missed <= 1);
        }
    }
}

TEST_CASE("Following the pads alone runs into buoys on every track") {
    for (const swoop_tracks::Track& track : swoop_tracks::kTracks) {
        CAPTURE(track.name);
        CHECK(Race(track, 5.0f, /*steerAround=*/false).hits >= 5);
    }
}
