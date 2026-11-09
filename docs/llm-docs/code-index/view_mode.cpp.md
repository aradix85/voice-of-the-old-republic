# view_mode.cpp (761 lines)

Source: `patches/Accessibility/view_mode.cpp`

(From view_mode.h.) View mode — freeze the character, keep the camera live.

Pressing B locks the player in place; A/D keep panning the camera natively (KOTOR's A/D rotates camera only — the character only snaps to camera direction when W/S commits forward motion). W/S then drive a virtual cursor instead of the character, the 3D listener is rerouted to the cursor, and Enter / Shift+Enter act on whatever the cursor is hovering over.

Hotkey B chosen because V is taken (Solo Mode in stock kotor.ini) and B is unbound by KOTOR's stock keymap.

## Declarations (in source order)

- L102 `void EnterViewMode()`
- L143 `void ExitViewMode()`
- L152 `void ToggleViewMode()`
- L159 `void DumpCameraStateProbe()`: Shift+B diagnostic: snapshot CClientOptions to the log.
- L209 `void StepCursor(float dt)`
- L301 `bool ResolveCursorRegionLabel(void* area, const Vector& cursor, std::string& outBuf, const char*& outSource, std::vector<acc::room_topology::ExitMark>& outExits)`: Tier order matches the walking adapter: landmark → friendly room name → shape cache.
- L358 `void AnnounceCursorRegion(void* area, const Vector& cursor)`: Per-tick region announce.
- L408 `void NarrateNearestObject(void* area, const Vector& cursor)`
- L497 `void PollEnter()`: Enter / Shift+Enter dispatch while view mode is active.
- L538 `void ProcessPendingDispatch()`: Runs at the top of Tick BEFORE the IsActive() gate — by design, this fires after PollEnter exited view mode synchronously.
- L591 `bool IsActive()`
- L593 `bool ConsumedEnterThisTick()`
- L599 `bool TryGetCursorPosition(Vector& out)`
- L605 `bool GetEffectiveOrientationYawDegrees(float& out)`
- L625 `bool GetEffectiveReferencePosition(Vector& out)`
- L635 `bool GetEffectiveViewpoint(Vector& outPos, float& outYawDegrees)`
- L645 `void PollWin32()`
- L726 `void Tick()`
