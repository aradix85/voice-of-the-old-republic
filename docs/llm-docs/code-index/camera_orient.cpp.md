# camera_orient.cpp (132 lines)

Source: `patches/Accessibility/camera_orient.cpp`

The N key: turns the camera to the exits of the player's region one by one (the nearest first, then clockwise), or to the next cardinal where the region has none. What to face is decided here (the rules are engine-free in camera_orient_pick.cpp); camera_orient_turn.cpp drives the turn.

## Declarations (in source order)

- L39 `bool CollectExitBearings(const Vector& playerPos, void*& outArea, int& outClusterId, std::vector<ExitBearing>& out)`: The region's exits as bearings from where the player stands: the real direction, not the octant word the label speaks.
- L61 `int LastExitIndex(const std::vector<ExitBearing>& exits, void* area, int clusterId)`
- L72 `float ChooseTarget(float curYawRad, const char*& mode)`: The heading this press turns to; `mode` names the rule for the log.
- L98 `void Tick()`
