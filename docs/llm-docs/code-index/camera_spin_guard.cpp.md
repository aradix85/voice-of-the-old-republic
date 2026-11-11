# camera_spin_guard.cpp (501 lines)

Source: `patches/Accessibility/camera_spin_guard.cpp`

The camera spin guard. After a load the cursor can park frozen at the screen edge, inside UpdateCamera's edge-turn band, and the camera spins for good; injected motion is ignored then (SetCursorPos and SendInput both failed), so the guard writes the engine's cursor to the viewport centre while the camera rotates with the cursor in the band. Two log-only diagnostics ride along: a cause-agnostic watch for uncommanded rotation and a tripwire for the camera yaw read that the guard relies on.

## Declarations (in source order)

- L85 `int RightClickHeld()`: The engine's rightClickHeld flag, 0 when unreadable.
- L92 `bool WriteCursorX(void* gui, uint32_t x)`: The engine's cursor x on the GuiManager.
- L102 `float NormDeg(float d)`
- L109 `float AngularDelta(float a, float b)`: Signed shortest delta a-b in (-180, 180].
- L159 `void EndSpinWatch(DWORD now, const char* why)`
- L177 `void EndEpisode(DWORD now, float endYaw)`
- L189 `bool CameraDrivenByNonPlayer()`: Something other than the player may move the camera: a cinematic or dialog shot or a blocking screen (IsForegroundUiBlocking tests the dialog first), or our own N snap-turn.
- L223 `bool StepWitness(float yawDeg, DWORD now, bool& have, float& prevYaw, DWORD& prevMs, float& stepDeg, float& rateDps)`: A yaw witness's step since its last sample.
- L247 `void SampleYaws(DWORD now, const Vector& playerPos, Sample& s)`: The three witnesses.
- L276 `void SampleCursor(Sample& s)`: The cursor against the edge band.
- L310 `bool ApplyEdgeGuard(const Sample& s)`: The fix: UpdateCamera's edge-turn (0x5f5e10) spins the camera while the cursor sits in the band.
- L318 `void TrackEpisode(DWORD now, const Sample& s, bool fired)`
- L345 `void ReportSpinTurn(DWORD now, const Sample& s, int due)`: One line per full uncommanded turn, with every input UpdateCamera turns on.
- L402 `void WatchUncommandedRotation(DWORD now, const Sample& s, bool commanding, bool notOurs)`: The cause-agnostic spin detector: the camera turning while the player commands nothing leaves a record whatever drove it.
- L438 `void CheckReadAnomaly(DWORD now, const Sample& s, bool commanding, bool notOurs)`: The quaternion and bearing yaws are only comparable in the ordinary follow pose (no cinematic, screen or snap-turn of ours, no turn input, a plausible distance), and a regression persists where a transient does not.
- L474 `void Tick()`
