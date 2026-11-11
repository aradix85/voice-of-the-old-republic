// The pull curve of the minigame aim assists (minigame_magnet).

#include "doctest.h"
#include "minigame_magnet.h"

using acc::minigame::MagnetGain;
using acc::minigame::MagnetParams;
using acc::minigame::MagnetStep;
using doctest::Approx;

namespace {
const MagnetParams kParams{0.1f, 0.5f, 2.0f};  // far gain, near gain, cap per tick
}

TEST_CASE("The pull is gentle at the edge and strong on the target") {
    CHECK(MagnetGain(0.0f, kParams) == Approx(0.1f));
    CHECK(MagnetGain(1.0f, kParams) == Approx(0.5f));
    CHECK(MagnetGain(0.5f, kParams) == Approx(0.2f));  // t squared: 0.1 + 0.25 * 0.4
}

TEST_CASE("Closeness outside 0..1 counts as the nearest end") {
    CHECK(MagnetGain(-3.0f, kParams) == Approx(0.1f));
    CHECK(MagnetGain(7.0f, kParams) == Approx(0.5f));
}

TEST_CASE("A step pulls against the error") {
    CHECK(MagnetStep(10.0f, 4.0f, 0.25f, kParams) == Approx(9.0f));
    CHECK(MagnetStep(10.0f, -4.0f, 0.25f, kParams) == Approx(11.0f));
    CHECK(MagnetStep(10.0f, 0.0f, 0.5f, kParams) == Approx(10.0f));
}

TEST_CASE("A step is capped per tick, both ways") {
    CHECK(MagnetStep(0.0f, 100.0f, 0.5f, kParams) == Approx(-2.0f));
    CHECK(MagnetStep(0.0f, -100.0f, 0.5f, kParams) == Approx(2.0f));
}
