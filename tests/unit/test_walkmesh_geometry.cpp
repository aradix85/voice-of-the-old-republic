// walkmesh_geometry: the wall filters, the segment-vs-wall test, the
// nearest walkable floor and the spot to stand on it.

#include "doctest.h"
#include "walkmesh_geometry.h"

using acc::engine::DistToPolygonXY;
using acc::engine::DropPortalEdges;
using acc::engine::DropSameRoomDuplicates;
using acc::engine::FloorSpotNear;
using acc::engine::FloorTri;
using acc::engine::IsWalkableMaterial;
using acc::engine::NearestWalkableFloor;
using acc::engine::OverWalkableFloor;
using acc::engine::PointInPolygonXY;
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

TEST_CASE("The spot to stand lies a step onto the floor, straight out from the point") {
    // The same desk: the spot is 0.6 m behind the edge the terminal is nearest to.
    const FloorTri floor[] = {
        Tri(-0.33f, -11.36f, 1.67f, -11.36f, 1.67f, -9.94f, 0, kMetal),
        Tri(-0.33f, -11.36f, 1.67f, -11.36f, 0.5f, -12.97f, 0, kNonWalk),
    };
    Vector out{};
    REQUIRE(FloorSpotNear(floor, 2, Vector{1.2f, -11.54f, 0.88f}, 3.0f, out));
    CHECK(out.x == Approx(1.2f));
    CHECK(out.y == Approx(-10.76f));
    CHECK(out.z == Approx(0.0f));
}

TEST_CASE("Where the floor is narrower than the step, the spot is its edge") {
    // A strip 0.4 m deep: a step of 0.6 m would leave it on the far side.
    const FloorTri floor[] = {
        Tri(0, 0, 4, 0, 4, 0.4f, 0, kMetal),
        Tri(0, 0, 4, 0.4f, 0, 0.4f, 0, kMetal),
    };
    Vector out{};
    REQUIRE(FloorSpotNear(floor, 2, Vector{2, -1, 0}, 3.0f, out));
    CHECK(out.x == Approx(2.0f));
    CHECK(out.y == Approx(0.0f));
}

TEST_CASE("A point on the floor, or with no floor near, has no spot to stand") {
    const FloorTri floor[] = {Tri(0, 0, 4, 0, 0, 4, 0, kMetal)};
    Vector out{};
    CHECK_FALSE(FloorSpotNear(floor, 1, Vector{1, 1, 0.5f}, 3.0f, out));
    CHECK_FALSE(FloorSpotNear(floor, 1, Vector{-5, 0, 0}, 3.0f, out));
    CHECK_FALSE(FloorSpotNear(nullptr, 0, Vector{-1, 0, 0}, 3.0f, out));
}

TEST_CASE("A point is over the walkable floor at whatever height") {
    const FloorTri floor[] = {
        Tri(0, 0, 4, 0, 0, 4, 10.0f, kMetal),
        Tri(0, 0, 4, 0, 4, -4, 10.0f, kNonWalk),
    };
    CHECK(OverWalkableFloor(floor, 2, Vector{1, 1, 0}));
    CHECK(OverWalkableFloor(floor, 2, Vector{1, 1, 10}));
    CHECK_FALSE(OverWalkableFloor(floor, 2, Vector{3, -1, 10}));   // the non-walkable face
    CHECK_FALSE(OverWalkableFloor(floor, 2, Vector{-1, 1, 10}));
    CHECK_FALSE(OverWalkableFloor(nullptr, 0, Vector{1, 1, 0}));
}

TEST_CASE("A point is inside a polygon or a distance from its outline") {
    // An L: the notch at the top right is outside.
    const Vector poly[] = {Vector{0, 0, 0}, Vector{4, 0, 0}, Vector{4, 2, 0},
                           Vector{2, 2, 0}, Vector{2, 4, 0}, Vector{0, 4, 0}};
    CHECK(PointInPolygonXY(poly, 6, 1, 1));
    CHECK(PointInPolygonXY(poly, 6, 1, 3));
    CHECK_FALSE(PointInPolygonXY(poly, 6, 3, 3));
    CHECK_FALSE(PointInPolygonXY(poly, 2, 1, 1));
    CHECK(DistToPolygonXY(poly, 6, 1, 1) == Approx(0.0f));
    CHECK(DistToPolygonXY(poly, 6, 3, 3) == Approx(1.0f));
    CHECK(DistToPolygonXY(poly, 6, 6, 1) == Approx(2.0f));
    CHECK(DistToPolygonXY(poly, 2, 1, 1) > 1e8f);
}

TEST_CASE("The middle of the Ebon Hawk's exit lies off the floor, 0.6 m in from the ramp's foot") {
    // The hold floor up to the foot of the ramp at y 49.04 and the ramp
    // itself, which is not walkable; the transition's centroid lies on it.
    const FloorTri floor[] = {
        Tri(64.28f, 43.0f, 68.73f, 43.0f, 68.73f, 49.04f, 1.94f, kMetal),
        Tri(64.28f, 43.0f, 68.73f, 49.04f, 64.28f, 49.04f, 1.94f, kMetal),
        FloorTri{Vector{63.71f, 54.54f, 4.5f}, Vector{64.28f, 49.04f, 1.94f},
                 Vector{68.73f, 49.04f, 1.94f}, kNonWalk},
    };
    const Vector centroid{66.26f, 51.42f, 3.07f};
    CHECK_FALSE(OverWalkableFloor(floor, 3, centroid));
    Vector out{};
    REQUIRE(FloorSpotNear(floor, 3, centroid, 5.0f, out));
    CHECK(out.x == Approx(66.26f));
    CHECK(out.y == Approx(48.44f));
    CHECK(out.z == Approx(1.94f));
}
