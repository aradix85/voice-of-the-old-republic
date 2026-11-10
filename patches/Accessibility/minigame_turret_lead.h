// The turret's predictive lead: where to point the gun so a bolt fired now
// meets the fighter. Engine-free.

#pragma once

#include "engine_offsets_types.h"

namespace acc::turret_game {

struct LeadSolution {
    Vector aimRel;     // gun-relative point to aim at
    Vector aimRelLin;  // the linear lead alone (LeadCheck diagnostic)
    float  t;          // bolt flight time to the intercept, 0 without lead
    float  dist;       // how far the lead moves the aim point
    float  frac;       // how much of the full lead the range gate lets through
};

// `p` is the fighter relative to the gun. `vel` is its smoothed relative
// velocity, or nullptr while unknown or when hits don't fly (hitscan): then
// the aim point is the fighter itself. `accel` is its centripetal
// acceleration, or nullptr while the estimate has not settled. `radius` is
// the fighter's hitbox radius (0 if unread); the bolt's speed and lifespan
// come from the player's gun bank.
LeadSolution SolveLead(const Vector& p, const Vector* vel, const Vector* accel,
                       float radius, float boltSpeed, float boltLife);

}  // namespace acc::turret_game
