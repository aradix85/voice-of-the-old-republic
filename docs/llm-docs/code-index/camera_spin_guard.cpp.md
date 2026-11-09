# camera_spin_guard.cpp (634 lines)

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
- L269 `void Tick()`
