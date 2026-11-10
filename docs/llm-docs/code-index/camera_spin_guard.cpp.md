# camera_spin_guard.cpp (569 lines)

Source: `patches/Accessibility/camera_spin_guard.cpp`

sources UpdateCamera feeds to AcclTurnCamera

## Declarations (in source order)

- L118 `T SafeRead(uintptr_t addr, T fallback)`
- L127 `T SafeReadOff(void* base, size_t off, T fallback)`
- L138 `bool SafeWriteOff(void* base, size_t off, T value)`
- L149 `float NormDeg(float d)`
- L156 `float AngularDelta(float a, float b)`: Signed shortest delta a-b in (-180, 180].
- L207 `void ResetSpinWatch()`
- L227 `void EndSpinWatch(DWORD now, const char* why)`
- L246 `void EndEpisode(DWORD now, float endYaw)`
- L262 `bool CameraDrivenByNonPlayer()`: True while something other than the player is entitled to move the camera: an engine cinematic / dialog shot, a blocking screen, or our own N-hotkey snap-turn.
- L297 `bool StepWitness(float yawDeg, DWORD now, bool& have, float& prevYaw, DWORD& prevMs, float& stepDeg, float& rateDps)`: A yaw witness's step since its last sample.
- L321 `void SampleYaws(DWORD now, const Vector& playerPos, Sample& s)`: The three witnesses.
- L350 `void SampleCursor(Sample& s)`: The cursor against the edge band.
- L381 `bool ApplyEdgeGuard(const Sample& s)`: The fix: UpdateCamera's edge-turn (0x5f5e10) spins the camera while the cursor sits in the band.
- L389 `void TrackEpisode(DWORD now, const Sample& s, bool fired)`
- L416 `void ReportSpinTurn(DWORD now, const Sample& s, int due)`: One line per full uncommanded turn, with every input UpdateCamera turns on.
- L470 `void WatchUncommandedRotation(DWORD now, const Sample& s, bool commanding, bool notOurs)`: The cause-agnostic spin detector: the camera turning while the player commands nothing leaves a record whatever drove it.
- L506 `void CheckReadAnomaly(DWORD now, const Sample& s, bool commanding, bool notOurs)`: The quaternion and bearing yaws are only comparable in the ordinary follow pose (no cinematic, screen or snap-turn of ours, no turn input, a plausible distance), and a regression persists where a transient does not.
- L542 `void Tick()`
