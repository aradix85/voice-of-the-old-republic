# view_mode.cpp (395 lines)

Source: `patches/Accessibility/view_mode.cpp`

(From view_mode.h.) View mode — freeze the character, keep the camera live.

Pressing B locks the player in place; A/D keep panning the camera natively (KOTOR's A/D rotates camera only — the character only snaps to camera direction when W/S commits forward motion). W/S then drive a virtual cursor instead of the character, the 3D listener is rerouted to the cursor, and Enter / Shift+Enter act on whatever the cursor is hovering over.

Hotkey B chosen because V is taken (Solo Mode in stock kotor.ini) and B is unbound by KOTOR's stock keymap.

## Declarations (in source order)

- L33 `void EnterViewMode()`
- L74 `void ExitViewMode()`
- L83 `void ToggleViewMode()`
- L90 `void DumpCameraStateProbe()`: Shift+B diagnostic: snapshot CClientOptions to the log.
- L140 `void StepCursor(float dt)`
- L232 `bool IsActive()`
- L234 `bool TryGetCursorPosition(Vector& out)`
- L240 `bool GetEffectiveOrientationYawDegrees(float& out)`
- L260 `bool GetEffectiveReferencePosition(Vector& out)`
- L269 `bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees)`
- L279 `void PollWin32()`
- L360 `void Tick()`
