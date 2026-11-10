# camera_orient_pick.cpp (80 lines)

Source: `patches/Accessibility/camera_orient_pick.cpp`

Engine-free choice of where N turns the camera (camera_orient_pick.h).

## Declarations (in source order)

- L15 `float AbsOffset(float yawRad, float curYawRad)`
- L21 `void AddExitBearing(std::vector<ExitBearing>& exits, const ExitBearing& e)`
- L28 `int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx)`
- L44 `int ChooseExit(const std::vector<ExitBearing>& exits, int lastIdx, float curYawRad, const char*& outWhy)`
- L67 `float NextCardinalCompassDeg(float compassDeg)`
- L73 `float CompassDegToEngineRad(float compassDeg)`
