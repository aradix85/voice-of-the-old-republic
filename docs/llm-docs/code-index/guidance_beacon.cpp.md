# guidance_beacon.cpp (261 lines)

Source: `patches/Accessibility/guidance_beacon.cpp`

(From guidance_beacon.h.) Audio beacon — drives Pillar 1 3D cues along a pathfinder waypoint sequence. Pillar 3 Mode B's "audio guide" half.

Cues: BeaconActive             — heartbeat between waypoints, 3D-positioned at the NEXT waypoint so pan/attenuation point the user that way. BeaconWaypointReached    — on reaching next waypoint; advances cursor. BeaconDestinationReached — after the final waypoint; disarms.

First cut is single-area only. If the destination is a transition, the engine swaps modules when the player walks through it; beacon state goes stale and the user re-arms in the new area.

Singleton — StartBeacon supersedes any prior. Ctrl+- toggles.

## Declarations (in source order)

- L62 `Vector CompressHeartbeatPosition(const Vector& target, const Vector& listener)`
- L100 `void EmitHeartbeat(const Vector& worldPos, const Vector& listenerPos)`: Fire the directional heartbeat.
- L118 `void EmitArrivalCue(acc::audio::NavCue cue, const char* tag)`: Fire an arrival confirmation.
- L136 `void SpeakNextSegment(const Vector& playerPos, const Vector& nextWp)`: Speak the current segment's distance + compass direction.
- L159 `void StartBeacon(const std::vector<Vector>& waypoints)`
- L174 `void CancelBeacon()`
- L183 `bool IsActive()`
- L187 `bool GetCurrentTarget(Vector& out)`
- L194 `void Tick()`
