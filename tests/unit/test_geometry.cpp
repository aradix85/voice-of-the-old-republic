// Distances (engine_offsets_types.h) and compass words (engine_compass).

#include "doctest.h"
#include "engine_compass.h"
#include "engine_offsets_types.h"

using acc::engine::ClockPosition;
using acc::engine::CompassToSector;
using acc::engine::EngineYawToCompass;
using doctest::Approx;

TEST_CASE("Distances on the floor plan ignore height") {
    const Vector a{0.0f, 0.0f, 0.0f};
    const Vector b{3.0f, 4.0f, 100.0f};
    CHECK(DistXY(a, b) == Approx(5.0f));
    CHECK(DistXYSq(a, b) == Approx(25.0f));
    CHECK(DistXYZSq(a, b) == Approx(10025.0f));
    CHECK(DistXY(b, a) == DistXY(a, b));
}

TEST_CASE("Engine yaw becomes compass degrees") {
    CHECK(EngineYawToCompass(0.0f) == Approx(90.0f));    // engine 0 = east
    CHECK(EngineYawToCompass(90.0f) == Approx(0.0f));    // north
    CHECK(EngineYawToCompass(180.0f) == Approx(270.0f)); // west
    CHECK(EngineYawToCompass(EngineYawToCompass(37.0f)) == Approx(37.0f));
}

TEST_CASE("Compass degrees become one of eight sectors") {
    CHECK(CompassToSector(0.0f) == 0);    // N
    CHECK(CompassToSector(22.4f) == 0);
    CHECK(CompassToSector(22.6f) == 1);   // NE
    CHECK(CompassToSector(90.0f) == 2);   // E
    CHECK(CompassToSector(337.4f) == 7);  // NW
    CHECK(CompassToSector(337.6f) == 0);
    CHECK(CompassToSector(359.0f) == 0);
}

TEST_CASE("Clock positions are relative to where the player faces") {
    // Facing east (engine yaw 0): ahead is +x, right is -y.
    CHECK(ClockPosition(0.0f, 1.0f, 0.0f) == 12);
    CHECK(ClockPosition(0.0f, 0.0f, -1.0f) == 3);
    CHECK(ClockPosition(0.0f, -1.0f, 0.0f) == 6);
    CHECK(ClockPosition(0.0f, 0.0f, 1.0f) == 9);
    // Facing north (engine yaw 90): east is to the right.
    CHECK(ClockPosition(90.0f, 1.0f, 0.0f) == 3);
}
