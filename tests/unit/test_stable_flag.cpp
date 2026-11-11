// StableFlag (stable_flag.h): a reading counts once it has held.

#include "doctest.h"
#include "stable_flag.h"

using acc::StableFlag;

TEST_CASE("The first reading is adopted without a change") {
    StableFlag f(200);
    CHECK_FALSE(f.Update(true, 1000));
    CHECK(f.Settled());
    CHECK_FALSE(f.Update(true, 5000));
}

TEST_CASE("A change counts after the quiet time, once") {
    StableFlag f(200);
    f.Update(false, 1000);
    CHECK_FALSE(f.Update(true, 1100));
    CHECK_FALSE(f.Update(true, 1299));
    CHECK_FALSE(f.Settled());
    CHECK(f.Update(true, 1300));
    CHECK(f.Settled());
    CHECK_FALSE(f.Update(true, 1400));
}

TEST_CASE("A flicker is not a change") {
    StableFlag f(200);
    f.Update(false, 1000);
    CHECK_FALSE(f.Update(true, 1100));
    CHECK_FALSE(f.Update(false, 1150));
    CHECK_FALSE(f.Update(false, 2000));
    CHECK_FALSE(f.Settled());
}

TEST_CASE("Every flip restarts the quiet time") {
    StableFlag f(200);
    f.Update(false, 1000);
    f.Update(true, 1100);
    f.Update(false, 1250);
    CHECK_FALSE(f.Update(true, 1300));
    CHECK_FALSE(f.Update(true, 1499));
    CHECK(f.Update(true, 1500));
}

TEST_CASE("The clock may wrap") {
    StableFlag f(200);
    f.Update(false, 0xFFFFFF00u);
    CHECK_FALSE(f.Update(true, 0xFFFFFFF0u));
    CHECK_FALSE(f.Update(true, 0x00000050u));
    CHECK(f.Update(true, 0x000000C0u));
}
