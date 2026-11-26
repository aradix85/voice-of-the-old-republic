# engine_input.h (262 lines)

Source: `patches/Accessibility/engine_input.h`

Engine input-code translation. Pure read-side; no engine state access.

## Declarations (in source order)

- L13 `const char* InputIndexName(int code)`: Human-readable name.
- L18 `int ManagerTranslateCode(int code)`: Translates the four KOTOR logical-action codes CSWGuiManager::HandleInputEvent rewrites before per-panel dispatch.
- L34 `bool ForceReacquireInput()`: Force a real SetActive(0)->SetActive(1) edge to re-Acquire the DirectInput devices unconditionally — the software equivalent of an alt-tab.
- L45 `bool ReleaseInput()`: Release the engine's DirectInput grab via SetActive(0) — the deactivate half.
- L64 `void RequestInputReacquire()`: Deferred-reacquire request, for focus/window events that arrive on the engine's message-pump thread but at a fragile point (inside the engine's own WM_ACTIVATEAPP / window-recreation handling).
- L73 `void RequestInputRelease()`: Counterpart for focus-LOSS: request that the next drain releases the engine's DirectInput grab (ReleaseInput).
- L78 `void DrainPendingReacquire()`: Drains a pending input-acquisition change (no-op if none).
- L94 `void InstallDirectInputMouseGuard()`: Install the no-mouse crash guard on CExoRawInputInternal::InitializeDirectInputMouse.
- L126 `void InstallMouseTeardownBlock()`: Stop a failed mouse device from taking the keyboard with it.
- L141 `void LogDirectInputStateIfChanged()`: ---- DirectInput state probe -------------------------------------------- Walks CExoInput -> CExoInputInternal -> CExoRawInputInternal and logs the active flag plus the interface / keyboard / mouse device pointers whe...
- L157 `void NoteEngineKeyEvent()`: ---- Engine keyboard liveness ------------------------------------------- Called from both input hooks (the in-world CClientExoAppInternal route and the CSWGuiManager route) whenever the ENGINE delivers a real key PRESS.
- L160 `unsigned long long LastEngineKeyEventMs()`: GetTickCount64() at the last NoteEngineKeyEvent, 0 if none this session.
- L168 `bool PollTurnAxis(float* out)`: The engine's own turn-axis value, exactly as UpdateCamera polls it every frame (CExoInput::PollInput(0x11c, class 0), clamped by the caller to [-1, 1] before it reaches AcclTurnCamera).
- L175 `bool ReadNativeStickAxes(float* x, float* y)`: KOTOR 2 only: the native right-stick pair UpdateCamera reads ahead of the screen-edge test (DirectInput controllers enumerated at start, 20% dead zone, first-seen baseline latch — camera-and-swoop.md "KOTOR 2 twin").

Also 28 constants or variables at namespace scope.
