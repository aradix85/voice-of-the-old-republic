// camera_orient_pick: which exit, or which cardinal, N turns the camera to.

#include <string>
#include <vector>

#include "camera_orient_pick.h"
#include "doctest.h"

using namespace acc::camera_orient;
using doctest::Approx;

namespace {

ExitBearing At(float deg) {
    return ExitBearing{deg * kDegToRad, Vector{0.0f, 0.0f, 0.0f}};
}

// Engine frame: East 0, North 90, West 180, South -90.
std::vector<ExitBearing> FourExits() {
    return {At(0.0f), At(90.0f), At(180.0f), At(-90.0f)};
}

}  // namespace

TEST_CASE("Exits within the arrival window of a kept one collapse into it") {
    std::vector<ExitBearing> exits;
    AddExitBearing(exits, At(10.0f));
    AddExitBearing(exits, At(11.0f));   // 1 degree apart: a turn would do nothing
    AddExitBearing(exits, At(-170.0f));
    AddExitBearing(exits, At(190.0f));  // the same bearing across the wrap
    CHECK(exits.size() == 2);
}

TEST_CASE("Clockwise is decreasing yaw in the engine frame") {
    const auto exits = FourExits();
    CHECK(NextExitClockwise(exits, 1) == 0);  // North -> East
    CHECK(NextExitClockwise(exits, 0) == 3);  // East -> South
    CHECK(NextExitClockwise(exits, 3) == 2);  // South -> West
    CHECK(NextExitClockwise(exits, 2) == 1);  // West -> North
    const std::vector<ExitBearing> one = {At(45.0f)};
    CHECK(NextExitClockwise(one, 0) == 0);
}

TEST_CASE("The first press faces the exit nearest the heading") {
    const char* why = "";
    CHECK(ChooseExit(FourExits(), -1, 80.0f * kDegToRad, why) == 1);
    CHECK(std::string(why) == "nearest");
    // Already facing it: the next one clockwise.
    CHECK(ChooseExit(FourExits(), -1, 90.0f * kDegToRad, why) == 0);
    CHECK(std::string(why) == "already-facing");
}

TEST_CASE("A press while still facing the last exit cycles on") {
    const char* why = "";
    // The turn to North landed 6 degrees short.
    CHECK(ChooseExit(FourExits(), 1, 84.0f * kDegToRad, why) == 0);
    CHECK(std::string(why) == "cycle");
    // Turned away from it since: back to the nearest.
    CHECK(ChooseExit(FourExits(), 1, 170.0f * kDegToRad, why) == 2);
    CHECK(std::string(why) == "nearest");
}

TEST_CASE("Without exits the press steps to the next cardinal") {
    CHECK(NextCardinalCompassDeg(10.0f) == Approx(90.0f));
    CHECK(NextCardinalCompassDeg(90.0f) == Approx(180.0f));
    CHECK(NextCardinalCompassDeg(359.0f) == Approx(0.0f));
    CHECK(NextCardinalCompassDeg(-10.0f) == Approx(0.0f));
    CHECK(CompassDegToEngineRad(0.0f) == Approx(90.0f * kDegToRad));   // North
    CHECK(CompassDegToEngineRad(90.0f) == Approx(0.0f));               // East
    CHECK(CompassDegToEngineRad(180.0f) == Approx(270.0f * kDegToRad)); // South
}
