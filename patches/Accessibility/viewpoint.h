// The frame spoken directions are relative to: the player's position and
// the CAMERA's yaw. KOTOR's turn keys orbit the camera without turning the
// character, whose own yaw only follows when a step is taken, so "at 2
// o'clock" must follow where the user is looking.

#pragma once

#include "engine_offsets.h"  // Vector

namespace acc::viewpoint {

// Camera yaw (engine frame); the character's until the camera yaw is known.
bool YawDegrees(float& out);

// Position and yaw in one call, so the clock and the distance of one spoken
// line come from the same frame. False when either read fails; callers drop
// the clock rather than speak a stale one.
bool Get(Vector& outPos, float& outYawDegrees);

}  // namespace acc::viewpoint
