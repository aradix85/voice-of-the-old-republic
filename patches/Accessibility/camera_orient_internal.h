// Shared between the camera_orient files: the camera heading read and the
// turn drive (camera_orient_turn.cpp) that Tick starts and ticks.

#pragma once

#include "camera_orient.h"
#include "camera_orient_pick.h"

namespace acc::camera_orient {

// The camera heading in the engine frame; false when it cannot be read.
bool ReadCurrentEngineYawRad(float& out);

bool TurnInFlight();
void TickTurn(void* camera);

// The snap, or the key-hold turn when this build cannot write the request.
void StartTurn(float targetEngineYawRad, float curYawRad, const char* mode);

// Logs where the last key-hold turn stopped, once its coast is over.
void LogLanding();

}  // namespace acc::camera_orient
