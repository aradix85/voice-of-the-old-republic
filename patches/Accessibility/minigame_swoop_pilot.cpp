#include "minigame_swoop_pilot.h"

#include <cmath>

namespace acc::swoop_pilot {

namespace {

// The share of kPilotStepU the pilot counts on when it asks whether there is
// time: the margin for an uneven tick.
constexpr float kPlanShare = 0.8f;
// Room kept between the bike's sphere and a rock's side.
constexpr float kSideSlackU = 1.5f;
// A rock is behind once the sphere is this far past its back.
constexpr float kBehindSlackU = 0.5f;
// How far from the middle of the lane the pilot steers; the tunnel clamps at 20.
constexpr float kLaneReachU = 19.5f;
// A rock is decided this many ticks before the bike could no longer cross it.
constexpr float kDecideAheadTicks = 2.0f;
// The line settles in two passes; the third is the proof that it has.
constexpr int kMaxPasses = 3;

float Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// One rock as the bike meets it: the strip it closes and when.
struct Meeting {
    float xl, xr;   // the bike's centre stays outside (xl, xr)
    float front;    // y at which the strip closes, one tick of travel early
    float ticks;    // ticks until then, 0 when alongside
};

// The side to pass on, or kUndecided while the rock is no obstacle to the way
// to the goal. [lo, hi] is what nearer rocks and the lane leave open.
Side ChooseSide(const Bike& bike, float goalX, float goalY, const Meeting& m, float lo, float hi,
                float planStep) {
    const bool leftOpen  = m.xl >= lo;
    const bool rightOpen = m.xr <= hi;
    if (!leftOpen && !rightOpen) return kUndecided;
    const float goal  = Clamp(goalX, lo, hi);
    const float reach = planStep * m.ticks;
    if (m.front > goalY) {
        // The pad comes first. The rock waits while the bike can still get
        // clear of it from the pad.
        if (goal <= m.xl || goal >= m.xr) return kUndecided;
        const float afterPad = planStep * (m.front - goalY) / bike.fwdPerTick;
        if ((leftOpen && goal - m.xl <= afterPad) || (rightOpen && m.xr - goal <= afterPad)) {
            return kUndecided;
        }
    } else {
        // Where the bike is when the strip closes, on its way to the goal.
        const float arrive = bike.x + Clamp(goal - bike.x, -reach, reach);
        if ((arrive <= m.xl && goal <= m.xl) || (arrive >= m.xr && goal >= m.xr)) {
            return kUndecided;
        }
    }
    // The side nearest the goal that the bike reaches in time; with neither in
    // time, the nearest side.
    const bool leftInTime  = leftOpen && bike.x - m.xl <= reach;
    const bool rightInTime = rightOpen && m.xr - bike.x <= reach;
    if (leftInTime && rightInTime) {
        return std::fabs(goal - m.xl) <= std::fabs(goal - m.xr) ? kPassLeft : kPassRight;
    }
    if (leftInTime) return kPassLeft;
    if (rightInTime) return kPassRight;
    if (!rightOpen) return kPassLeft;
    if (!leftOpen) return kPassRight;
    return std::fabs(bike.x - m.xl) <= std::fabs(bike.x - m.xr) ? kPassLeft : kPassRight;
}

}  // namespace

float LineAroundRocks(const Bike& bike, float goalX, float goalY, const Rock* rocks, int count,
                      int8_t* sideBySlot, int& blockingSlot) {
    int blocking = count;   // index of the nearest rock that narrows
    const float planStep = kPilotStepU * kPlanShare;
    const float clear    = bike.radius + kSideSlackU;
    float lo = bike.laneX - kLaneReachU;
    float hi = bike.laneX + kLaneReachU;
    // A rock that narrows [lo, hi] can turn a nearer rock into an obstacle,
    // hence the passes.
    for (int pass = 0; pass < kMaxPasses; ++pass) {
        bool narrowed = false;
        for (int i = 0; i < count; ++i) {
            const Rock& r = rocks[i];
            if (r.y1 + bike.radius + kBehindSlackU < bike.y) continue;   // behind
            Meeting m;
            m.xl    = r.x0 - clear;
            m.xr    = r.x1 + clear;
            m.front = r.y0 - bike.radius - bike.fwdPerTick;
            m.ticks = m.front > bike.y ? (m.front - bike.y) / bike.fwdPerTick : 0.0f;
            if (m.ticks > (m.xr - m.xl) / planStep + kDecideAheadTicks) break;   // too far to decide
            int8_t& side = sideBySlot[r.slot];
            if (side == kUndecided) side = ChooseSide(bike, goalX, goalY, m, lo, hi, planStep);
            // A side that nearer rocks have closed is left alone: nearer wins.
            if (side == kPassLeft && m.xl >= lo && m.xl < hi) {
                hi = m.xl;
            } else if (side == kPassRight && m.xr <= hi && m.xr > lo) {
                lo = m.xr;
            } else {
                continue;
            }
            narrowed = true;
            if (i < blocking) blocking = i;
        }
        if (!narrowed) break;
    }
    blockingSlot = blocking < count ? rocks[blocking].slot : -1;
    return blocking < count ? Clamp(goalX, lo, hi) : goalX;
}

}  // namespace acc::swoop_pilot
