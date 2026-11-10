# camera_orient_turn.cpp (399 lines)

Source: `patches/Accessibility/camera_orient_turn.cpp`

The two turn drives: the snap (the camera's one-shot desired-orientation request) and the key-hold turn that takes over when the camera ignores it.

## Declarations (in source order)

- L95 `bool WriteDesiredOrientation(float deg)`
- L106 `bool ReadDesiredOrientationPending(bool& pending)`: False on no address / fault.
- L116 `void SendKey(WORD scan, bool down)`
- L123 `void ReleaseAndDisarm(const char* reason, float curYawRad)`
- L158 `bool ReadCurrentEngineYawRad(float& out)`: Prefer camera_announce's position-derived yaw — atan2(player - camera) is single-valued, the quaternion path returns antipodal readings 360° apart and breaks the arrival check.
- L177 `void LogLanding()`: One line per turn once the coast has finished: where the camera actually stopped against the target.
- L191 `bool ArmKeyTurn(float targetEngineYawRad, float curYawRad, const char* mode)`: Start a key-hold turn toward the target.
- L236 `void TickKeyTurn(void* camera)`: In-flight key-hold turn: the predictive-release state machine.
- L309 `bool StartSnap(float targetEngineYawRad, float curYawRad, const char* mode)`: Issue the snap.
- L331 `void TickSnap(void* camera)`: In-flight snap: wait for the camera to take the request, then for the heading to show it.
- L393 `bool IsActive()`: Covers the window g_rot.active alone misses: the rising-edge tick itself, since camera_announce::Tick runs before us in core_tick and would otherwise speak the pre-rotation sector on the frame N is pressed.
