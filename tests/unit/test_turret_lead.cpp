// minigame_turret_lead: where to point the turret so a bolt meets the fighter.

#include <cmath>

#include "doctest.h"
#include "minigame_turret_lead.h"

using acc::turret_game::LeadSolution;
using acc::turret_game::SolveLead;

namespace {

float Len(const Vector& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }

}  // namespace

TEST_CASE("without a velocity the aim point is the fighter") {
    const Vector p = {0.0f, 100.0f, 0.0f};
    const LeadSolution s = SolveLead(p, nullptr, nullptr, 5.0f, 300.0f, 3.0f);
    CHECK(s.t == 0.0f);
    CHECK(s.aimRel.x == p.x);
    CHECK(s.aimRel.y == p.y);
    CHECK(s.aimRel.z == p.z);
}

TEST_CASE("a crossing fighter is led to a true intercept") {
    const Vector p = {0.0f, 100.0f, 0.0f};
    const Vector v = {10.0f, 0.0f, 0.0f};
    // Radius 0: no range gate, the full lead.
    const LeadSolution s = SolveLead(p, &v, nullptr, 0.0f, 100.0f, 3.0f);
    CHECK(s.frac == 1.0f);
    CHECK(s.t == doctest::Approx(std::sqrt(10000.0f / 9900.0f)).epsilon(1e-4));
    CHECK(s.aimRel.x == doctest::Approx(10.0f * s.t).epsilon(1e-4));
    CHECK(s.aimRel.y == doctest::Approx(100.0f));
    // At the intercept the bolt has flown exactly as far as the aim point is.
    CHECK(Len(s.aimRel) == doctest::Approx(100.0f * s.t).epsilon(1e-4));
    CHECK(s.aimRelLin.x == s.aimRel.x);
}

TEST_CASE("a close slow fighter gets no lead from the range gate") {
    const Vector p = {0.0f, 50.0f, 0.0f};
    const Vector v = {1.0f, 0.0f, 0.0f};
    // Drift during flight is far below 0.75 radius.
    const LeadSolution s = SolveLead(p, &v, nullptr, 10.0f, 300.0f, 3.0f);
    CHECK(s.t > 0.0f);
    CHECK(s.frac == 0.0f);
    CHECK(s.aimRel.x == doctest::Approx(0.0f));
}

TEST_CASE("an intercept past the bolt's lifespan gets no lead") {
    const Vector p = {0.0f, 1000.0f, 0.0f};
    const Vector v = {10.0f, 0.0f, 0.0f};
    const LeadSolution s = SolveLead(p, &v, nullptr, 0.0f, 100.0f, 3.0f);
    CHECK(s.t == 0.0f);
    CHECK(s.aimRel.y == p.y);
}

TEST_CASE("the turn term bends the lead but never beyond the linear lead") {
    const Vector p = {0.0f, 100.0f, 0.0f};
    const Vector v = {20.0f, 0.0f, 0.0f};
    const Vector a = {0.0f, -50.0f, 0.0f};
    const LeadSolution lin = SolveLead(p, &v, nullptr, 0.0f, 100.0f, 3.0f);
    const LeadSolution curve = SolveLead(p, &v, &a, 0.0f, 100.0f, 3.0f);
    CHECK(curve.aimRelLin.x == lin.aimRel.x);
    CHECK(curve.aimRel.y < lin.aimRel.y);  // bent toward the turn
    const Vector bend = {curve.aimRel.x - p.x, curve.aimRel.y - p.y, curve.aimRel.z - p.z};
    CHECK(Len(bend) < 2.5f * lin.dist);
}
