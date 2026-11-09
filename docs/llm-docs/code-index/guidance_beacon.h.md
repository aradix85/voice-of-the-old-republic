# guidance_beacon.h (49 lines)

Source: `patches/Accessibility/guidance_beacon.h`

Audio beacon — drives Pillar 1 3D cues along a pathfinder waypoint sequence. Pillar 3 Mode B's "audio guide" half.

Cues: BeaconActive             — heartbeat between waypoints, 3D-positioned at the NEXT waypoint so pan/attenuation point the user that way. BeaconWaypointReached    — on reaching next waypoint; advances cursor. BeaconDestinationReached — after the final waypoint; disarms.

First cut is single-area only. If the destination is a transition, the engine swaps modules when the player walks through it; beacon state goes stale and the user re-arms in the new area.

Singleton — StartBeacon supersedes any prior. Ctrl+- toggles.

## Declarations (in source order)

- L34 `void StartBeacon(const std::vector<Vector>& waypoints)`: Empty `waypoints` cancels.
- L36 `void CancelBeacon()`
- L38 `bool IsActive()`
- L42 `bool GetCurrentTarget(Vector& out)`: World position of the waypoint currently being steered toward (false when no beacon armed / path drained).
- L46 `void Tick()`: Per-tick driver.

Also 2 constants or variables at namespace scope.
