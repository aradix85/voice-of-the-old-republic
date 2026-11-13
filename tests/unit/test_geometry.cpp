// Distances (engine_offsets_types.h) and compass words (engine_compass).

#include "doctest.h"
#include "engine_compass.h"
#include "engine_offsets_types.h"

using acc::engine::ClockPosition;
using acc::engine::CompassToSector;
using acc::engine::EngineYawToCompass;
using acc::engine::VectorCompass;
using acc::engine::VectorEngineYaw;
using acc::engine::VectorSector;
using doctest::Approx;

TEST_CASE("Distances on the floor plan ignore height") {
    const Vector a{0.0f, 0.0f, 0.0f};
    const Vector b{3.0f, 4.0f, 100.0f};
    CHECK(DistXY(a, b) == Approx(5.0f));
    CHECK(DistXYSq(a, b) == Approx(25.0f));
    CHECK(DistXYZSq(a, b) == Approx(10025.0f));
    CHECK(DistXY(b, a) == DistXY(a, b));
}

TEST_CASE("A door stands in the way only between the two ends and near the line") {
    const Vector player{102.4f, 118.3f, 2.25f};
    const Vector door{102.6f, 119.1f, 2.25f};
    const Vector north{102.4f, 120.7f, 0.0f};   // the region behind the door
    const Vector south{102.3f, 113.6f, 0.0f};   // the region the player stands in
    CHECK(StandsBetweenXY(door, player, north, 1.0f));
    CHECK_FALSE(StandsBetweenXY(door, player, south, 1.0f));     // behind the player

    const Vector a{0.0f, 0.0f, 0.0f};
    const Vector b{10.0f, 0.0f, 0.0f};
    CHECK(StandsBetweenXY(Vector{5.0f, 0.9f, 0.0f}, a, b, 1.0f));
    CHECK_FALSE(StandsBetweenXY(Vector{5.0f, 1.6f, 0.0f}, a, b, 1.0f));   // a door in the side wall
    CHECK_FALSE(StandsBetweenXY(Vector{12.0f, 0.0f, 0.0f}, a, b, 1.0f));  // past the far end
    CHECK_FALSE(StandsBetweenXY(a, a, b, 1.0f));                          // at the near end
    CHECK_FALSE(StandsBetweenXY(Vector{1.0f, 0.0f, 0.0f}, a, a, 1.0f));   // no segment
}

TEST_CASE("A spoken distance is the nearest whole metre") {
    CHECK(WholeMetres(0.0f) == 0);
    CHECK(WholeMetres(0.49f) == 0);
    CHECK(WholeMetres(0.5f) == 1);
    CHECK(WholeMetres(12.4f) == 12);
    CHECK(WholeMetres(12.6f) == 13);
}

TEST_CASE("Something elsewhere is never 0 metres away") {
    CHECK(WholeMetresAtLeastOne(0.0f) == 1);
    CHECK(WholeMetresAtLeastOne(0.2f) == 1);
    CHECK(WholeMetresAtLeastOne(1.4f) == 1);
    CHECK(WholeMetresAtLeastOne(7.5f) == 8);
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

TEST_CASE("A floor-plan vector has an engine yaw, a bearing and a sector") {
    CHECK(VectorEngineYaw(1.0f, 0.0f) == Approx(0.0f));     // east
    CHECK(VectorEngineYaw(0.0f, 1.0f) == Approx(90.0f));    // north
    CHECK(VectorEngineYaw(0.0f, -1.0f) == Approx(270.0f));  // never negative
    CHECK(VectorCompass(0.0f, 1.0f) == Approx(0.0f));
    CHECK(VectorCompass(1.0f, 0.0f) == Approx(90.0f));
    CHECK(VectorCompass(-1.0f, 0.0f) == Approx(270.0f));
    CHECK(VectorSector(0.0f, 1.0f) == 0);    // N
    CHECK(VectorSector(1.0f, 1.0f) == 1);    // NE
    CHECK(VectorSector(1.0f, 0.0f) == 2);    // E
    CHECK(VectorSector(1.0f, -1.0f) == 3);   // SE
    CHECK(VectorSector(0.0f, -1.0f) == 4);   // S
    CHECK(VectorSector(-1.0f, -1.0f) == 5);  // SW
    CHECK(VectorSector(-1.0f, 0.0f) == 6);   // W
    CHECK(VectorSector(-1.0f, 1.0f) == 7);   // NW
}

TEST_CASE("Degrees and radians convert both ways") {
    CHECK(180.0f * kDegToRad == Approx(kPi));
    CHECK(kPi * kRadToDeg == Approx(180.0f));
}
