// save_points: the save-variable format of the exit memory. Saves written by
// earlier builds must keep reading the same, so the format is pinned here.

#include <string>
#include <vector>

#include "doctest.h"
#include "save_points.h"

namespace sp = acc::save_points;
using sp::Spot;

TEST_CASE("Parse reads plain spots (ACC_VISIT)") {
    const std::vector<Spot> s = sp::Parse("12,-34;0,5");
    REQUIRE(s.size() == 2);
    CHECK(s[0].x == 12);
    CHECK(s[0].y == -34);
    CHECK(s[0].value.empty());
    CHECK(s[1].x == 0);
    CHECK(s[1].y == 5);
}

TEST_CASE("Parse reads spots with a value (ACC_EXIT)") {
    const std::vector<Spot> s = sp::Parse("10,20=tar_m02aa;-3,4=tar_m03ae");
    REQUIRE(s.size() == 2);
    CHECK(s[0].value == "tar_m02aa");
    CHECK(s[1].x == -3);
    CHECK(s[1].value == "tar_m03ae");
}

TEST_CASE("Parse stops at the first malformed entry") {
    CHECK(sp::Parse("1,2;oops;3,4").size() == 1);
    CHECK(sp::Parse("1;2,3").empty());
    CHECK(sp::Parse("1,2x3,4").size() == 1);
    CHECK(sp::Parse("").empty());
    CHECK(sp::Parse(nullptr).empty());
}

TEST_CASE("Parse skips an entry with an empty value") {
    const std::vector<Spot> s = sp::Parse("1,2=;3,4=b");
    REQUIRE(s.size() == 1);
    CHECK(s[0].value == "b");
}

TEST_CASE("Format writes the saved format, which Parse reads back") {
    const std::vector<Spot> v = {{1, 2, ""}, {-3, 4, "mod"}};
    CHECK(sp::Format(v) == "1,2;-3,4=mod");
    const std::vector<Spot> back = sp::Parse(sp::Format(v).c_str());
    REQUIRE(back.size() == 2);
    CHECK(back[0].x == 1);
    CHECK(back[1].value == "mod");
    CHECK(sp::Format({}).empty());
}

TEST_CASE("At rounds to whole metres") {
    const Spot s = sp::At(Vector{1.4f, -2.6f, 9.0f}, "x");
    CHECK(s.x == 1);
    CHECK(s.y == -3);
    CHECK(s.value == "x");
}

TEST_CASE("Nearest finds the closest spot within the radius") {
    const std::vector<Spot> v = {{0, 0, "a"}, {5, 0, "b"}, {20, 0, "c"}};
    const Spot* s = sp::Nearest(v, Vector{4.0f, 0.0f, 0.0f}, 6.0f);
    REQUIRE(s != nullptr);
    CHECK(s->value == "b");
    CHECK(sp::Nearest(v, Vector{12.0f, 0.0f, 0.0f}, 6.0f) == nullptr);
    const std::vector<Spot> none;
    CHECK(sp::Nearest(none, Vector{0.0f, 0.0f, 0.0f}, 6.0f) == nullptr);
}

TEST_CASE("Put replaces the same spot and puts the new one last") {
    std::vector<Spot> v = {{0, 0, "a"}, {10, 0, "b"}};
    sp::Put(v, Spot{1, 1, "c"}, 2, 60);
    REQUIRE(v.size() == 2);
    CHECK(v[0].value == "b");
    CHECK(v[1].value == "c");
}

TEST_CASE("Put drops the oldest spots beyond the maximum") {
    std::vector<Spot> v;
    for (long i = 0; i < 5; ++i) sp::Put(v, Spot{i * 10, 0, std::to_string(i)}, 2, 3);
    REQUIRE(v.size() == 3);
    CHECK(v[0].value == "2");
    CHECK(v[2].value == "4");
}
