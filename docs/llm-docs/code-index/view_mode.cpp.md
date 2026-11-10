# view_mode.cpp (274 lines)

Source: `patches/Accessibility/view_mode.cpp`

View mode (B): a virtual cursor walked with W/A/S/D while the character is frozen, which names the regions and objects it passes and takes Enter for the object under it. The listener follows the cursor through the CExoSound::SetListenerPosition detour (hooks.toml), which runs after the engine's own write; the camera keys keep panning natively.

## Declarations (in source order)

- L36 `void EnterViewMode()`
- L64 `void ExitViewMode()`
- L72 `void DumpCameraStateProbe()`: Shift+B diagnostic: a snapshot of CClientOptions in the log.
- L100 `bool KeyDown(int vk)`
- L106 `void StepCursor(float dt)`: Move the cursor along the camera's heading (W/S) and its right (D/A), at walking pace, stopping short of real walls (surfaces, not portal seams).
- L158 `const char* ToggleBlockReason()`: Why the B toggle must wait, or nullptr.
- L190 `bool IsActive()`
- L192 `bool TryGetCursorPosition(Vector& out)`
- L201 `bool GetEffectiveOrientationYawDegrees(float& out)`: The camera's facing in normal play and in view mode alike: the camera is what the user sweeps while the character stands still.
- L212 `bool GetEffectiveReferencePosition(Vector& out)`: In view mode the cursor, not the frozen body, is where the user "is" for every distance and bearing spoken.
- L217 `bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees)`
- L227 `void PollWin32()`
- L251 `void Tick()`
