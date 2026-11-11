# minigame_swoop_pilot.cpp (114 lines)

Source: `patches/Accessibility/minigame_swoop_pilot.cpp`

(From minigame_swoop_pilot.h.) The line of the swoop race's automatic pilot around the obstacles of KOTOR 1 (buoys: boxes to steer around). Engine-free: world coordinates in, the lateral position to steer to out.

The pilot moves the bike sideways by at most kPilotStepU per tick and plans with less, so a decision that says "there is time" has a margin. An obstacle is decided late, when the bike could still just cross its whole width, and the side then sticks until the obstacle is behind: until that moment the bike is free to chase the accelerator pads.

## Declarations (in source order)

- L23 `float Clamp(float v, float lo, float hi)`
- L36 `Side ChooseSide(const Bike& bike, float goalX, float goalY, const Meeting& m, float lo, float hi, float planStep)`: The side to pass on, or kUndecided while the rock is no obstacle to the way to the goal.
- L74 `float LineAroundRocks(const Bike& bike, float goalX, float goalY, const Rock* rocks, int count, int8_t* sideBySlot, int& blockingSlot)`
