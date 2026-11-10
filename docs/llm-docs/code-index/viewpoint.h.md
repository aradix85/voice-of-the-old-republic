# viewpoint.h (21 lines)

Source: `patches/Accessibility/viewpoint.h`

The frame spoken directions are relative to: the player's position and the CAMERA's yaw. KOTOR's turn keys orbit the camera without turning the character, whose own yaw only follows when a step is taken, so "at 2 o'clock" must follow where the user is looking.

## Declarations (in source order)

- L13 `bool YawDegrees(float& out)`: Camera yaw (engine frame); the character's until the camera yaw is known.
- L18 `bool Get(Vector& outPos, float& outYawDegrees)`: Position and yaw in one call, so the clock and the distance of one spoken line come from the same frame.
