# view_mode.cpp (753 lines)

Source: `patches/Accessibility/view_mode.cpp`

(From view_mode.h.) View mode — freeze the character, keep the camera live.

Pressing B locks the player in place; A/D keep panning the camera natively (KOTOR's A/D rotates camera only — the character only snaps to camera direction when W/S commits forward motion). W/S then drive a virtual cursor instead of the character, the 3D listener is rerouted to the cursor, and Enter / Shift+Enter act on whatever the cursor is hovering over.

Hotkey B chosen because V is taken (Solo Mode in stock kotor.ini) and B is unbound by KOTOR's stock keymap.

## Declarations (in source order)

- L99 `void EnterViewMode()`
- L140 `void ExitViewMode()`
- L149 `void ToggleViewMode()`
- L156 `void DumpCameraStateProbe()`: Shift+B diagnostic: snapshot CClientOptions to the log.
- L206 `void StepCursor(float dt)`
- L298 `bool ResolveCursorRegionLabel(void* area, const Vector& cursor, std::string& outBuf, const char*& outSource)`: Tier order matches the walking adapter: landmark → friendly room name → shape cache.
- L352 `void AnnounceCursorRegion(void* area, const Vector& cursor)`: Per-tick region announce.
- L401 `void NarrateNearestObject(void* area, const Vector& cursor)`
- L490 `void PollEnter()`: Enter / Shift+Enter dispatch while view mode is active.
- L531 `void ProcessPendingDispatch()`: Runs at the top of Tick BEFORE the IsActive() gate — by design, this fires after PollEnter exited view mode synchronously.
- L584 `bool IsActive()`
- L586 `bool ConsumedEnterThisTick()`
- L592 `bool TryGetCursorPosition(Vector& out)`
- L598 `bool GetEffectiveOrientationYawDegrees(float& out)`
- L618 `bool GetEffectiveReferencePosition(Vector& out)`
- L627 `bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees)`
- L637 `void PollWin32()`
- L718 `void Tick()`
