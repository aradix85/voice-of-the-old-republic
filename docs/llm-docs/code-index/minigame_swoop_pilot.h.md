# minigame_swoop_pilot.h (46 lines)

Source: `patches/Accessibility/minigame_swoop_pilot.h`

The line of the swoop race's automatic pilot around the obstacles of KOTOR 1 (buoys: boxes to steer around). Engine-free: world coordinates in, the lateral position to steer to out.

The pilot moves the bike sideways by at most kPilotStepU per tick and plans with less, so a decision that says "there is time" has a margin. An obstacle is decided late, when the bike could still just cross its whole width, and the side then sticks until the obstacle is behind: until that moment the bike is free to chase the accelerator pads.

## Declarations (in source order)

- L21 `struct Rock`: An obstacle's collision box in world coordinates.
- L27 `struct Bike`
- L35 `enum Side`: The side an obstacle is passed on.
- L42 `float LineAroundRocks(const Bike& bike, float goalX, float goalY, const Rock* rocks, int count, int8_t* sideBySlot, int& blockingSlot)`: Where to steer this tick to reach (goalX, goalY), the next accelerator pad, without touching a rock.

Also 1 constants or variables at namespace scope.
