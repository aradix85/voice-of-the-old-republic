// floor_puzzle_plates: tag slots, plate shape and point-versus-plate geometry.

#include "doctest.h"
#include "floor_puzzle_plates.h"

#include <cmath>

using namespace acc::floor_puzzle;
using doctest::Approx;

namespace {

// A 2 x 2 m axis-aligned plate from (10, 20) to (12, 22).
Plate Square() {
    Plate p;
    p.poly[0] = Vector{10, 20, 1};
    p.poly[1] = Vector{12, 20, 1};
    p.poly[2] = Vector{12, 22, 1};
    p.poly[3] = Vector{10, 22, 1};
    p.vertCount = 4;
    ComputeCenterAndSpan(p);
    return p;
}

}  // namespace

TEST_CASE("Trigger and placeable tags map to plate slots") {
    CHECK(TrigSlotForTag("kFloorPanel01") == 0);
    CHECK(TrigSlotForTag("KFLOORPANEL09") == 8);
    CHECK(TrigSlotForTag("kPanelReset") == kResetIndex);
    CHECK(TrigSlotForTag("kFloorPanel10") == -1);
    CHECK(TrigSlotForTag("kFloorPanel00") == -1);
    CHECK(TrigSlotForTag("kDoorTrigger") == -1);
    CHECK(PlcSlotForTag("FloorPanel05") == 4);
    CHECK(PlcSlotForTag("kFloorPanel05") == -1);
    CHECK(PlcSlotForTag("FloorPanelReset") == -1);
}

TEST_CASE("A plate's centre and span come from its polygon") {
    const Plate p = Square();
    CHECK(p.center.x == Approx(11.0f));
    CHECK(p.center.y == Approx(21.0f));
    CHECK(p.center.z == Approx(1.0f));
    CHECK(p.minX == Approx(10.0f));
    CHECK(p.maxX == Approx(12.0f));
    CHECK(p.minY == Approx(20.0f));
    CHECK(p.maxY == Approx(22.0f));
}

TEST_CASE("Distance to a plate is zero inside and edge distance outside") {
    const Plate p = Square();
    CHECK(PointInPoly(p, 11, 21));
    CHECK_FALSE(PointInPoly(p, 13, 21));
    CHECK(DistToPlate(p, 11, 21) == Approx(0.0f));
    CHECK(DistToPlate(p, 13, 21) == Approx(1.0f));
    CHECK(DistToPlate(p, 13, 23) == Approx(std::sqrt(2.0f)));

    Plate degenerate;
    degenerate.vertCount = 2;
    CHECK_FALSE(PointInPoly(degenerate, 0, 0));
    CHECK(DistToPlate(degenerate, 0, 0) > 1e8f);
}

TEST_CASE("The offset to a plate counts only the axes outside its span") {
    const Plate p = Square();
    SpanOffset beside = OffsetToSpan(p, Vector{11, 18.5f, 0});   // south of it
    CHECK(beside.north == Approx(1.5f));
    CHECK(beside.east == Approx(0.0f));

    SpanOffset diagonal = OffsetToSpan(p, Vector{9, 23, 0});      // north-west
    CHECK(diagonal.north == Approx(-1.0f));
    CHECK(diagonal.east == Approx(1.0f));

    SpanOffset on = OffsetToSpan(p, Vector{10.5f, 21.5f, 0});
    CHECK(on.north == 0.0f);
    CHECK(on.east == 0.0f);
}
