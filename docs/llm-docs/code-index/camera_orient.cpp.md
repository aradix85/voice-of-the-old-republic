# camera_orient.cpp (243 lines)

Source: `patches/Accessibility/camera_orient.cpp`

(From camera_orient.h.) Camera-orient hotkey (N).

- Face an exit of the player's region (room_topology's exit points, real bearing from the player): the one nearest the camera heading, then clockwise on each further press while the player stays in the region and still faces the exit N last turned to. - No region/exits → advance one cardinal CW (N → E → S → W → N). A turn that stays inside one compass sector is silent on purpose: nothing heard means the heading was already right (user decision 2026-10-30).

Drive mechanism, first choice: the snap — the free-roam camera's one-shot desired-orientation request (a yaw the camera takes on its next update and then clears; nothing in the engine writes it). Turns in a frame or two. Fallback when the active camera never takes it (combat camera) or the build has no address for it: the key-hold turn below.

Key-hold turn: synthesised keypresses of the player's *bound* turn key via SendInput with KEYEVENTF_SCANCODE, so the engine's own UpdateCamera runs its full poll-axis → turn → apply pass exactly as if the key were held. The scancode comes from engine_keymap::TurnScancode() (read from swkotor.ini [Keymapping]), so a turn rebind is honoured — the only change from the old hardcoded A/D. (Calling the engine's AcclTurnCamera directly from our out-of-band tick was tried and does not reliably move the chase-cam.) A closed-loop arrival check via the engine's own yaw read releases the key.

camera_announce stays muted while IsActive() is true and announces the final direction once the rotation lands, if the sector changed.

## Declarations (in source order)

- L27 `float CompassDegToEngineRad(float compassDeg)`
- L60 `bool CollectExitBearings(const Vector& playerPos, void*& outArea, int& outClusterId, std::vector<ExitBearing>& out)`
- L96 `int NextExitClockwise(const std::vector<ExitBearing>& exits, int fromIdx)`: Next exit clockwise from `fromYawRad` (engine frame is CCW+, so clockwise = decreasing yaw).
- L118 `int ChooseExit(const std::vector<ExitBearing>& exits, void* area, int clusterId, float curYawRad, const char*& outWhy)`: Pick the exit to turn to: - still in the region of the last N turn and still facing that exit → the next one clockwise (the cycle); - otherwise the exit nearest the camera heading, however small the offset — N lines t...
- L150 `float NextCardinalCompassDeg(float currentCompassDeg)`: Next cardinal in CW order (N → E → S → W → N).
- L163 `void Tick()`
