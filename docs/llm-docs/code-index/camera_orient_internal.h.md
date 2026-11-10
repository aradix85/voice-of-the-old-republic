# camera_orient_internal.h (61 lines)

Source: `patches/Accessibility/camera_orient_internal.h`

Shared between the camera_orient files: the angle helpers, the camera heading read, and the two turn drives (camera_orient_turn.cpp) that Tick starts and ticks.

## Declarations (in source order)

- L17 `struct Rotation`: Single rotation at a time.
- L30 `struct Snap`
- L47 `inline float NormaliseRad(float r)`
- L53 `bool ReadCurrentEngineYawRad(float& out)`
- L54 `void LogLanding()`
- L55 `bool ArmKeyTurn(float targetEngineYawRad, float curYawRad, const char* mode)`
- L56 `void TickKeyTurn(void* camera)`
- L57 `bool StartSnap(float targetEngineYawRad, float curYawRad, const char* mode)`
- L58 `void TickSnap(void* camera)`

Also 7 constants or variables at namespace scope.
