# view_mode.h (75 lines)

Source: `patches/Accessibility/view_mode.h`

View mode — freeze the character, keep the camera live.

Pressing B locks the player in place; A/D keep panning the camera natively (KOTOR's A/D rotates camera only — the character only snaps to camera direction when W/S commits forward motion). W/S then drive a virtual cursor instead of the character, the 3D listener is rerouted to the cursor, and Enter / Shift+Enter act on whatever the cursor is hovering over.

Hotkey B chosen because V is taken (Solo Mode in stock kotor.ini) and B is unbound by KOTOR's stock keymap.

## Declarations (in source order)

- L29 `bool IsActive()`
- L35 `bool ConsumedEnterThisTick()`: Read-and-clear flag set when PollEnter handled this tick's Enter rising edge.
- L38 `bool TryGetCursorPosition(Vector& out)`: Cursor world position.
- L46 `bool GetEffectiveOrientationYawDegrees(float& out)`: Camera yaw (engine frame), in BOTH normal play and view mode, with character yaw as the first-ticks fallback.
- L54 `bool GetEffectiveReferencePosition(Vector& out)`: The position spatial announcements must be measured FROM: the view-mode cursor while view mode is active, the player creature otherwise.
- L61 `bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees)`: The full viewpoint an "at X o'clock, Y metres" line is relative to: GetEffectiveReferencePosition + GetEffectiveOrientationYawDegrees in one call, so the clock and the distance in a single spoken line can never be mea...
- L65 `void PollWin32()`: B / Shift+B edge detection.
- L72 `void Tick()`: Per-tick cursor step, hover narration, region announce, Enter dispatch.

Also 1 constants or variables at namespace scope.
