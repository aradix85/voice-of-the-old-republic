// turnback_zones: the store per area, the crossing test and the outlines as
// walls.

#include "doctest.h"
#include "turnback_zones.h"

namespace tb = acc::guidance::turnback;

namespace {

// A strip from x 10 to 12, y 0 to 20: the rim of a map, with the playable
// floor to its west.
const Vector kStrip[] = {Vector{10, 0, 0}, Vector{12, 0, 0}, Vector{12, 20, 0},
                         Vector{10, 20, 0}};

int g_areaA = 0;
int g_areaB = 0;

}  // namespace

TEST_CASE("A zone is kept once per area and forgotten with it") {
    tb::ForArea(&g_areaA);
    CHECK(tb::Add(kStrip, 4));
    CHECK_FALSE(tb::Add(kStrip, 4));         // known already
    CHECK_FALSE(tb::Add(kStrip, 2));         // no polygon
    CHECK_FALSE(tb::Add(nullptr, 4));
    CHECK(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));
    tb::ForArea(&g_areaA);                   // the same area keeps it
    CHECK(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));
    tb::ForArea(&g_areaB);
    CHECK_FALSE(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));
}

TEST_CASE("A walk into a zone crosses it; one out of it or to a target inside does not") {
    tb::ForArea(&g_areaB);
    tb::ForArea(&g_areaA);
    REQUIRE(tb::Add(kStrip, 4));
    CHECK(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));        // through it
    CHECK_FALSE(tb::Crosses(Vector{5, 5, 0}, Vector{9, 15, 0}));  // beside it
    CHECK_FALSE(tb::Crosses(Vector{11, 5, 0}, Vector{5, 5, 0}));  // out of it
    CHECK_FALSE(tb::Crosses(Vector{5, 5, 0}, Vector{11, 5, 0}));  // to a target inside
    // Round the end of the strip: neither leg crosses.
    CHECK_FALSE(tb::Crosses(Vector{5, 5, 0}, Vector{5, 25, 0}));
    CHECK_FALSE(tb::Crosses(Vector{5, 25, 0}, Vector{20, 25, 0}));
}

TEST_CASE("Standing zones are walked round like learned ones and go as a set") {
    // A second strip further east, from x 30 to 32.
    const Vector kFar[] = {Vector{30, 0, 0}, Vector{32, 0, 0}, Vector{32, 20, 0},
                           Vector{30, 20, 0}};
    tb::ForArea(&g_areaB);
    tb::ForArea(&g_areaA);
    REQUIRE(tb::Add(kStrip, 4));
    REQUIRE(tb::AddStanding(kFar, 4));
    CHECK_FALSE(tb::AddStanding(kFar, 4));                           // known already
    CHECK(tb::Crosses(Vector{25, 5, 0}, Vector{40, 5, 0}));          // the standing one
    CHECK(tb::CrossesStanding(Vector{25, 5, 0}, Vector{40, 5, 0}));
    CHECK(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));           // the learned one
    CHECK_FALSE(tb::CrossesStanding(Vector{5, 5, 0}, Vector{20, 5, 0}));
    CHECK_FALSE(tb::CrossesStanding(Vector{25, 5, 0}, Vector{31, 5, 0}));  // target inside

    tb::ClearStanding();
    CHECK_FALSE(tb::Crosses(Vector{25, 5, 0}, Vector{40, 5, 0}));
    CHECK(tb::Crosses(Vector{5, 5, 0}, Vector{20, 5, 0}));           // the learned one stays
    CHECK(tb::AddStanding(kFar, 4));                                 // and it can come back
    CHECK(tb::CrossesStanding(Vector{25, 5, 0}, Vector{40, 5, 0}));
}

TEST_CASE("The outlines come as walls, without the zone an end stands in") {
    tb::ForArea(&g_areaB);
    tb::ForArea(&g_areaA);
    REQUIRE(tb::Add(kStrip, 4));
    std::vector<acc::engine::WallEdge> walls;
    tb::AppendWalls(Vector{5, 5, 0}, Vector{20, 5, 0}, walls);
    REQUIRE(walls.size() == 4);
    CHECK(walls[3].a.x == doctest::Approx(10.0f));   // the closing edge
    CHECK(walls[3].a.y == doctest::Approx(20.0f));
    CHECK(walls[3].b.y == doctest::Approx(0.0f));
    walls.clear();
    tb::AppendWalls(Vector{11, 5, 0}, Vector{20, 5, 0}, walls);
    CHECK(walls.empty());
}
