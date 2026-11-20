// The health warnings of a fight (health_bands).

#include "doctest.h"
#include "health_bands.h"

using acc::combat::HealthBand;
using acc::combat::HealthWarnings;

TEST_CASE("Health is in a band at half and at a quarter") {
    CHECK(HealthBand(323, 323) == 0);
    CHECK(HealthBand(162, 323) == 0);
    CHECK(HealthBand(161, 323) == 1);
    CHECK(HealthBand(81, 323) == 1);
    CHECK(HealthBand(80, 323) == 2);
    CHECK(HealthBand(6, 323) == 2);
}

TEST_CASE("A creature that is down, or cannot be read, is in no band") {
    CHECK(HealthBand(0, 323) == 0);
    CHECK(HealthBand(-11, 42) == 0);
    CHECK(HealthBand(10, 0) == 0);
}

TEST_CASE("A warning comes once per band, on the way down") {
    HealthWarnings w;
    CHECK(w.Sank(1, 300, 323) == 0);
    CHECK(w.Sank(1, 150, 323) == 1);
    CHECK(w.Sank(1, 140, 323) == 0);
    CHECK(w.Sank(1, 60, 323) == 2);
    CHECK(w.Sank(1, 6, 323) == 0);
}

TEST_CASE("A blow through both bands warns once, for the deeper one") {
    HealthWarnings w;
    CHECK(w.Sank(1, 300, 323) == 0);
    CHECK(w.Sank(1, 40, 323) == 2);
}

TEST_CASE("Healing out of a band makes it tellable again") {
    HealthWarnings w;
    CHECK(w.Sank(1, 60, 323) == 2);
    CHECK(w.Sank(1, 200, 323) == 0);
    CHECK(w.Sank(1, 150, 323) == 1);
}

TEST_CASE("Each creature has its own bands, and a new fight starts clean") {
    HealthWarnings w;
    CHECK(w.Sank(1, 60, 323) == 2);
    CHECK(w.Sank(2, 100, 255) == 1);
    w.Clear();
    CHECK(w.Sank(1, 60, 323) == 2);
}
