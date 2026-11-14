// walkmesh_geometry: the wall filters, the segment-vs-wall test and the
// nearest walkable floor.

#include "doctest.h"
#include "walkmesh_geometry.h"

using acc::engine::DropPortalEdges;
using acc::engine::DropSameRoomDuplicates;
using acc::engine::FloorTri;
using acc::engine::IsWalkableMaterial;
using acc::engine::NearestWalkableFloor;
using acc::engine::SegmentCrossesWalkmesh;
using acc::engine::TriEdge;
using acc::engine::WallEdge;
using doctest::Approx;

namespace {

WallEdge Wall(float ax, float ay, float az, float bx, float by, float bz, int room) {
    return WallEdge{Vector{ax, ay, az}, Vector{bx, by, bz}, room, 0};
}

constexpr int kNonWalk = 7;   // surfacemat.2da rows
constexpr int kMetal   = 10;

// A level floor triangle at height z.
FloorTri Tri(float ax, float ay, float bx, float by, float cx, float cy, float z, int material) {
    return FloorTri{Vector{ax, ay, z}, Vector{bx, by, z}, Vector{cx, cy, z}, material};
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

TEST_CASE("Only the surfacemat rows with walk set are walkable") {
    CHECK(IsWalkableMaterial(kMetal));
    CHECK(IsWalkableMaterial(30));  // Trigger
    CHECK_FALSE(IsWalkableMaterial(kNonWalk));
    CHECK_FALSE(IsWalkableMaterial(0));
    CHECK_FALSE(IsWalkableMaterial(-1));
    CHECK_FALSE(IsWalkableMaterial(31));
    CHECK_FALSE(IsWalkableMaterial(64));
}

TEST_CASE("A point on the walkable floor is its own nearest floor") {
    const FloorTri floor[] = {Tri(0, 0, 4, 0, 0, 4, 1.0f, kMetal)};
    Vector out{};
    REQUIRE(NearestWalkableFloor(floor, 1, Vector{1, 1, 1.5f}, 3.0f, out));
    CHECK(out.x == Approx(1.0f));
    CHECK(out.y == Approx(1.0f));
    CHECK(out.z == Approx(1.0f));
}

TEST_CASE("Off the floor, the nearest floor is on the nearest edge") {
    // The reception desk of the Taris Sith base: the terminal lies on the
    // desk, the floor behind it comes within 0.18 m, the floor in front of it
    // stays 1.43 m away, and the desk top itself is non-walkable floor.
    const FloorTri floor[] = {
        Tri(-0.33f, -11.36f, 1.67f, -11.36f, 1.67f, -9.94f, 0, kMetal),   // behind
        Tri(0.04f, -12.97f, 1.27f, -12.97f, 0.5f, -14.0f, 0, kMetal),     // in front
        Tri(-0.33f, -11.36f, 1.67f, -11.36f, 0.5f, -12.97f, 0, kNonWalk), // the desk
    };
    const Vector terminal{0.24f, -11.54f, 0.88f};
    Vector out{};
    REQUIRE(NearestWalkableFloor(floor, 3, terminal, 3.0f, out));
    CHECK(out.x == Approx(0.24f));
    CHECK(out.y == Approx(-11.36f));
    CHECK(out.z == Approx(0.0f));
}

TEST_CASE("Floor out of range or on another storey is no floor") {
    const FloorTri floor[] = {Tri(0, 0, 4, 0, 0, 4, 0, kMetal)};
    Vector out{};
    CHECK_FALSE(NearestWalkableFloor(floor, 1, Vector{-5, 0, 0}, 3.0f, out));
    CHECK(NearestWalkableFloor(floor, 1, Vector{-2, 0, 0}, 3.0f, out));
    CHECK(out.x == Approx(0.0f));
    CHECK_FALSE(NearestWalkableFloor(floor, 1, Vector{1, 1, 4}, 3.0f, out));
    CHECK_FALSE(NearestWalkableFloor(nullptr, 0, Vector{1, 1, 0}, 3.0f, out));
}
