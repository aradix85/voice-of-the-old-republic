# camera_orient_turn.cpp (319 lines)

Source: `patches/Accessibility/camera_orient_turn.cpp`

The two turn drives. First choice is the snap: the camera's own one-shot desired-orientation request, which turns it within a frame. When the active camera never reads that request (the combat camera, an unmapped address), the key-hold turn takes over: the player's bound turn key is injected as a DirectInput scancode, so the engine's own UpdateCamera runs its poll-axis, AcclTurnCamera, apply sequence as if the key were held. Calling AcclTurnCamera from outside does not move the chase cam: its accumulator is only consumed inside the engine's own per-frame pass.

## Declarations (in source order)

- L91 `bool WriteDesiredOrientation(float deg)`
- L102 `bool ReadDesiredOrientationPending(bool& pending)`: False on no address or a fault.
- L112 `void ReleaseAndDisarm(const char* reason, float curYawRad)`
- L128 `bool ArmKeyTurn(float targetEngineYawRad, float curYawRad, const char* mode)`: False when already at the target (no key to hold).
- L154 `void TickKeyTurn(void* camera)`
- L204 `bool StartSnap(float targetEngineYawRad, float curYawRad, const char* mode)`: False when the request cannot be written (no address on this build, or a fault).
- L217 `void TickSnap(void* camera)`: Waits for the camera to take the request, then for the heading to show it; either wait running out hands the rest to the key-hold turn.
- L273 `bool ReadCurrentEngineYawRad(float& out)`: camera_announce's position-derived yaw first: atan2(player - camera) is single-valued.
- L282 `bool TurnInFlight()`
- L286 `void TickTurn(void* camera)`
- L294 `void StartTurn(float targetEngineYawRad, float curYawRad, const char* mode)`
- L302 `void LogLanding()`: One line per key-hold turn once the coast is over: where the camera really stopped.
- L314 `bool IsActive()`: Also true on the press's own tick, before a turn is armed: camera_announce ticks first in core_tick and would speak the old sector on that frame.
