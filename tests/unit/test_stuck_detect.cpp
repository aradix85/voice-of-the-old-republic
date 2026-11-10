// stuck_detect: the windowed stuck test and the probe's two triggers; and
// the point-to-segment distance they and floor_puzzle_plates share.

#include "doctest.h"
#include "stuck_detect.h"

using acc::audio::CirclingTrigger;
using acc::audio::NoProgressTrigger;
using acc::audio::StuckWindow;
using doctest::Approx;

namespace {

constexpr uint64_t kTick = 40;  // ms, a typical frame cadence

// Feeds positions x(t) along +X every kTick from t0 for durationMs.
template <typename Fn>
void Feed(StuckWindow& w, uint64_t t0, uint64_t durationMs, Fn x) {
    for (uint64_t t = t0; t <= t0 + durationMs; t += kTick) {
        w.Push(Vector{x(t), 0.0f, 0.0f}, t);
    }
}

}  // namespace

TEST_CASE("The point-to-segment distance clamps to the segment's ends") {
    const Vector a{0, 0, 0}, b{4, 0, 0};
    CHECK(DistXYToSegmentSq(Vector{2, 3, 9}, a, b) == Approx(9.0f));   // height ignored
    CHECK(DistXYToSegmentSq(Vector{-3, 4, 0}, a, b) == Approx(25.0f));  // past a
    CHECK(DistXYToSegmentSq(Vector{7, 0, 0}, a, b) == Approx(9.0f));    // past b
    CHECK(DistXYToSegmentSq(Vector{1, 1, 0}, a, a) == Approx(2.0f));    // a point
}

TEST_CASE("The stuck window seeds, skips frame noise, then judges") {
    StuckWindow w;
    CHECK(w.Push(Vector{0, 0, 0}, 1000) == StuckWindow::Step::Seeded);
    CHECK(w.Push(Vector{0, 0, 0}, 1005) == StuckWindow::Step::Skipped);
    CHECK(w.Push(Vector{0, 0, 0}, 1040) == StuckWindow::Step::Judged);
    CHECK_FALSE(w.Stuck());  // 40 ms of history is not a verdict
}

TEST_CASE("Walking is not stuck; standing still is, after 300 ms") {
    StuckWindow walk;
    Feed(walk, 1000, 2000, [](uint64_t t) { return 0.002f * (t - 1000); });  // 2 m/s
    CHECK_FALSE(walk.Stuck());
    CHECK(walk.Speed() == Approx(2.0f).epsilon(0.05));

    StuckWindow still;
    Feed(still, 1000, 280, [](uint64_t) { return 5.0f; });
    CHECK_FALSE(still.Stuck());
    Feed(still, 1320, 40, [](uint64_t) { return 5.0f; });
    CHECK(still.Stuck());
    CHECK(still.StuckSinceMs() != 0);
}

TEST_CASE("Bouncing against a wall nets nothing and counts as stuck") {
    StuckWindow w;
    Feed(w, 1000, 1000, [](uint64_t t) { return (t / kTick) % 2 ? 0.3f : 0.0f; });
    CHECK(w.Stuck());
}

TEST_CASE("Stuck holds in the dead band and clears on real progress") {
    StuckWindow w;
    Feed(w, 1000, 600, [](uint64_t) { return 0.0f; });
    REQUIRE(w.Stuck());
    // 1 m/s sits between the 0.8 enter and 1.2 exit speeds.
    Feed(w, 1640, 1000, [](uint64_t t) { return 0.001f * (t - 1640); });
    CHECK(w.Stuck());
    const float x0 = 0.001f * 1000;
    Feed(w, 2680, 1000, [x0](uint64_t t) { return x0 + 0.002f * (t - 2680); });
    CHECK_FALSE(w.Stuck());
    CHECK(w.StuckSinceMs() == 0);
}

TEST_CASE("After a reset the first verdict ignores the stale samples") {
    StuckWindow w;
    Feed(w, 1000, 2000, [](uint64_t t) { return 0.002f * (t - 1000); });
    w.Reset();
    w.Push(Vector{100, 0, 0}, 900000);
    w.Push(Vector{100.05f, 0, 0}, 900040);
    CHECK(w.SpanMs() == 40);
    CHECK(w.NetMetres() == Approx(0.05f));
    CHECK_FALSE(w.Stuck());
}

TEST_CASE("No progress fires once after 5 s within 0.5 m, re-arms on a move") {
    NoProgressTrigger trig;
    int fired = 0;
    for (uint64_t t = 1000; t <= 12000; t += 100) {
        fired += trig.Update(Vector{0.1f, 0, 0}, t, /*walking=*/true) ? 1 : 0;
    }
    CHECK(fired == 1);
    CHECK_FALSE(trig.Update(Vector{1.0f, 0, 0}, 12100, true));  // moved: re-armed
    CHECK_FALSE(trig.Update(Vector{1.0f, 0, 0}, 16000, true));
    CHECK(trig.Update(Vector{1.0f, 0, 0}, 17200, true));
    CHECK_FALSE(trig.Update(Vector{1.0f, 0, 0}, 30000, false));  // stopped walking
}

TEST_CASE("Circling fires once; walking somewhere does not") {
    CirclingTrigger circle;
    int fired = 0;
    // Back and forth over 2 m at 2 m/s: lots of path, no net progress.
    for (uint64_t t = 1000; t <= 7000; t += 100) {
        const uint64_t phase = (t - 1000) % 2000;
        const float x = phase < 1000 ? 0.002f * phase : 2.0f - 0.002f * (phase - 1000);
        fired += circle.Update(Vector{x, 0, 0}, t, true) ? 1 : 0;
    }
    CHECK(fired == 1);
    CHECK(circle.PathMetres() >= 5.0f);
    CHECK(circle.NetMetres() <= 2.5f);

    CirclingTrigger straight;
    fired = 0;
    for (uint64_t t = 1000; t <= 12000; t += 100) {
        fired += straight.Update(Vector{0.002f * (t - 1000), 0, 0}, t, true) ? 1 : 0;
    }
    CHECK(fired == 0);
}
