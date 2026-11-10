// walkmesh_geometry: the wall filters and the segment-vs-wall test.

#include "doctest.h"
#include "walkmesh_geometry.h"

using acc::engine::DropPortalEdges;
using acc::engine::DropSameRoomDuplicates;
using acc::engine::SegmentCrossesWalkmesh;
using acc::engine::TriEdge;
using acc::engine::WallEdge;
using doctest::Approx;

namespace {

WallEdge Wall(float ax, float ay, float az, float bx, float by, float bz, int room) {
    return WallEdge{Vector{ax, ay, az}, Vector{bx, by, bz}, room, 0};
}

}  // namespace

TEST_CASE("A wall across the path blocks it at the crossing") {
    const WallEdge walls[] = {Wall(5, -1, 0, 5, 1, 0, 0)};
    Vector hit{};
    REQUIRE(SegmentCrossesWalkmesh(walls, 1, Vector{0, 0, 0}, Vector{10, 0, 0}, hit));
    CHECK(hit.x == Approx(5.0f));
    CHECK(hit.y == Approx(0.0f));
}

TEST_CASE("Parallel walls and walls past the end do not block") {
    const WallEdge walls[] = {Wall(0, 1, 0, 10, 1, 0, 0), Wall(12, -1, 0, 12, 1, 0, 0)};
    Vector hit{};
    CHECK_FALSE(SegmentCrossesWalkmesh(walls, 2, Vector{0, 0, 0}, Vector{10, 0, 0}, hit));
    CHECK_FALSE(SegmentCrossesWalkmesh(walls, 2, Vector{3, 3, 0}, Vector{3, 3, 0}, hit));
    CHECK_FALSE(SegmentCrossesWalkmesh(nullptr, 0, Vector{0, 0, 0}, Vector{10, 0, 0}, hit));
}

TEST_CASE("The nearest of two walls is the hit") {
    const WallEdge walls[] = {Wall(8, -1, 0, 8, 1, 0, 0), Wall(3, -1, 0, 3, 1, 0, 0)};
    Vector hit{};
    REQUIRE(SegmentCrossesWalkmesh(walls, 2, Vector{0, 0, 0}, Vector{10, 0, 0}, hit));
    CHECK(hit.x == Approx(3.0f));
}

TEST_CASE("A wall on another floor blocks only in a 2D test") {
    const WallEdge walls[] = {Wall(5, -1, 0, 5, 1, 0, 0)};
    Vector hit{};
    const Vector a{0, 0, 4}, b{10, 0, 4};  // 4 m up, a ground-floor wall below
    CHECK_FALSE(SegmentCrossesWalkmesh(walls, 1, a, b, hit));
    CHECK(SegmentCrossesWalkmesh(walls, 1, a, b, hit, /*ignoreZ=*/true));
}

TEST_CASE("A wall matched by another room's triangle side is a portal") {
    WallEdge walls[] = {
        Wall(0, 0, 0, 4, 0, 0, 0),  // room 1 continues beyond it (reversed side)
        Wall(0, 5, 0, 4, 5, 0, 0),  // only room 0's own side matches: a real wall
        Wall(0, 9, 0, 4, 9, 0, 0),  // room 1's side is 2 cm off: a real wall
    };
    const TriEdge sides[] = {
        TriEdge{Vector{4, 0, 0.001f}, Vector{0, 0, 0}, 1},
        TriEdge{Vector{0, 5, 0}, Vector{4, 5, 0}, 0},
        TriEdge{Vector{0, 9.02f, 0}, Vector{4, 9.02f, 0}, 1},
    };
    REQUIRE(DropPortalEdges(walls, 3, sides, 3) == 2);
    CHECK(walls[0].a.y == Approx(5.0f));  // order kept
    CHECK(walls[1].a.y == Approx(9.0f));
}

TEST_CASE("Same-room duplicates go, walls of two floors stay") {
    WallEdge dup[] = {Wall(0, 0, 0, 4, 0, 0, 0), Wall(4, 0, 0, 0, 0, 0, 0)};
    CHECK(DropSameRoomDuplicates(dup, 2) == 1);

    // A step: the flat foot edge and the slanted face edge share the foot.
    WallEdge step[] = {Wall(0, 0, 0, 4, 0, 0, 0), Wall(0, 0, 0, 4, 0, 2.25f, 0)};
    CHECK(DropSameRoomDuplicates(step, 2) == 1);

    WallEdge floors[] = {Wall(0, 0, 0, 4, 0, 0, 0), Wall(0, 0, 3, 4, 0, 3, 0)};
    CHECK(DropSameRoomDuplicates(floors, 2) == 2);

    WallEdge rooms[] = {Wall(0, 0, 0, 4, 0, 0, 0), Wall(0, 0, 0, 4, 0, 0, 1)};
    CHECK(DropSameRoomDuplicates(rooms, 2) == 2);
}
