# camera_orient_pick.h (54 lines)

Source: `patches/Accessibility/camera_orient_pick.h`

Engine-free choice of where N turns the camera: the angle helpers, the exit to face among the region's exits, and the next cardinal when there are none. Angles are in the engine frame (East = 0, counter-clockwise positive) unless named compass (North = 0, clockwise).

## Declarations (in source order)

- L20 `inline float NormaliseRad(float r)`: To (-pi, pi].
- L27 `struct ExitBearing`: One exit of the player's region: its bearing from the player and its point.
- L34 `void AddExitBearing(std::vector<ExitBearing>& exits, const ExitBearing& e)`: Adds `e` unless a kept exit lies within kArrivalRad of its bearing: a turn between the two would do nothing.
- L38 `int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx)`: The next exit clockwise from exits[fromIdx]; fromIdx itself only when it is the only one.
- L45 `int ChooseExit(const std::vector<ExitBearing>& exits, int lastIdx, float curYawRad, const char*& outWhy)`: The exit to turn to.
- L49 `float NextCardinalCompassDeg(float compassDeg)`: The next cardinal clockwise (N, E, S, W) after compassDeg.
- L51 `float CompassDegToEngineRad(float compassDeg)`

Also 2 constants or variables at namespace scope.
