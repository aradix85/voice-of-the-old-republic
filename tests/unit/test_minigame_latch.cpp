// The latch of the swoop race and the turret game (minigame_latch).

#include "doctest.h"
#include "minigame_latch.h"

using acc::minigame::kExitDebounceTicks;
using acc::minigame::Latch;
using acc::minigame::LatchInput;
using acc::minigame::LatchStep;
using acc::minigame::StepLatch;

namespace {

int g_gameA, g_gameB, g_vtable;  // only their addresses are used

LatchInput Ours(void* game) { return LatchInput{game, &g_vtable, true, true}; }
LatchInput Other(void* game, bool latchedAlive) {
    return LatchInput{game, &g_vtable, false, latchedAlive};
}
LatchInput NoGame(bool latchedAlive) {
    return LatchInput{nullptr, nullptr, false, latchedAlive};
}

Latch Entered() {
    Latch latch;
    REQUIRE(StepLatch(latch, Ours(&g_gameA)) == LatchStep::Entered);
    return latch;
}

}  // namespace

TEST_CASE("Nothing, or a minigame of another kind, leaves the latch idle") {
    Latch latch;
    CHECK(StepLatch(latch, NoGame(false)) == LatchStep::Idle);
    CHECK(StepLatch(latch, Other(&g_gameA, false)) == LatchStep::Idle);
    CHECK_FALSE(latch.Active());
}

TEST_CASE("A minigame of ours is latched with its vtable") {
    const Latch latch = Entered();
    CHECK(latch.Active());
    CHECK(latch.game == &g_gameA);
    CHECK(latch.vtable == &g_vtable);
}

TEST_CASE("The latch holds while the area is silent and the struct is intact") {
    Latch latch = Entered();
    for (int i = 0; i < 3 * kExitDebounceTicks; ++i) {
        REQUIRE(StepLatch(latch, NoGame(true)) == LatchStep::Running);
    }
    CHECK(latch.game == &g_gameA);
}

TEST_CASE("Both sources gone ends the minigame after the debounce") {
    Latch latch = Entered();
    for (int i = 1; i < kExitDebounceTicks; ++i) {
        REQUIRE(StepLatch(latch, NoGame(false)) == LatchStep::Running);
    }
    CHECK(StepLatch(latch, NoGame(false)) == LatchStep::Exited);
    CHECK_FALSE(latch.Active());
    CHECK(StepLatch(latch, NoGame(false)) == LatchStep::Idle);
}

TEST_CASE("A gap shorter than the debounce is forgotten") {
    Latch latch = Entered();
    for (int i = 1; i < kExitDebounceTicks; ++i) StepLatch(latch, NoGame(false));
    CHECK(StepLatch(latch, Ours(&g_gameA)) == LatchStep::Running);
    CHECK(latch.ticksLost == 0);
}

TEST_CASE("A new struct of ours mid-game is re-latched") {
    Latch latch = Entered();
    CHECK(StepLatch(latch, Ours(&g_gameB)) == LatchStep::Running);
    CHECK(latch.game == &g_gameB);
}

TEST_CASE("Another kind of minigame in the area ends ours, intact struct or not") {
    Latch latch = Entered();
    for (int i = 1; i < kExitDebounceTicks; ++i) {
        REQUIRE(StepLatch(latch, Other(&g_gameB, true)) == LatchStep::Running);
    }
    CHECK(StepLatch(latch, Other(&g_gameB, true)) == LatchStep::Exited);
}
