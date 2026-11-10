# camera_orient_internal.h (24 lines)

Source: `patches/Accessibility/camera_orient_internal.h`

Shared between the camera_orient files: the camera heading read and the turn drive (camera_orient_turn.cpp) that Tick starts and ticks.

## Declarations (in source order)

- L12 `bool ReadCurrentEngineYawRad(float& out)`: The camera heading in the engine frame; false when it cannot be read.
- L14 `bool TurnInFlight()`
- L15 `void TickTurn(void* camera)`
- L18 `void StartTurn(float targetEngineYawRad, float curYawRad, const char* mode)`: The snap, or the key-hold turn when this build cannot write the request.
- L21 `void LogLanding()`: Logs where the last key-hold turn stopped, once its coast is over.
