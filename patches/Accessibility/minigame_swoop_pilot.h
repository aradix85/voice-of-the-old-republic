// The line of the swoop race's automatic pilot around the obstacles of
// KOTOR 1 (buoys: boxes to steer around). Engine-free: world coordinates in,
// the lateral position to steer to out.
//
// The pilot moves the bike sideways by at most kPilotStepU per tick and plans
// with less, so a decision that says "there is time" has a margin. An obstacle
// is decided late, when the bike could still just cross its whole width, and
// the side then sticks until the obstacle is behind: until that moment the
// bike is free to chase the accelerator pads.

#pragma once

#include <cstdint>

namespace acc::swoop_pilot {

// Sideways travel per tick while the pilot steers.
constexpr float kPilotStepU = 5.0f;

// An obstacle's collision box in world coordinates. The track runs along +y.
struct Rock {
    int   slot;   // the obstacle's identity; indexes the side memory
    float x0, x1;
    float y0, y1;
};

struct Bike {
    float x, y;
    float laneX;       // world x of the middle of the lane
    float radius;      // collision sphere
    float fwdPerTick;  // forward travel per tick, above zero
};

// The side an obstacle is passed on.
enum Side : int8_t { kUndecided = 0, kPassLeft = -1, kPassRight = 1 };

// Where to steer this tick to reach (goalX, goalY), the next accelerator pad,
// without touching a rock. `rocks` is sorted by y0, nearest first. `sideBySlot`
// holds one Side per obstacle slot and is written when a side is chosen; zero
// it at the start of a race. `blockingSlot` is the nearest rock that moves the
// line, or -1 when the line is the goal itself.
float LineAroundRocks(const Bike& bike, float goalX, float goalY, const Rock* rocks, int count,
                      int8_t* sideBySlot, int& blockingSlot);

}  // namespace acc::swoop_pilot
