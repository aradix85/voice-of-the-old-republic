# engine_keyboard_resync.cpp (346 lines)

Source: `patches/Accessibility/engine_keyboard_resync.cpp`

Keyboard transition-stream re-synchronisation — see the header for the contract and gui-and-input-internals.md ("keyboard_transition_stream") for the engine model. Decompiled 2026-09-14 on both games.

The engine side, in one paragraph. CExoInputInternal::GetEvents runs once per frame from ProcessInput. It calls CExoRawInputInternal:: GetKeyboardBuffer, which allocates a 256-entry batch and fills it with IDirectInputDevice8::GetDeviceData (DIDEVICEOBJECTDATA, 0x14 bytes each: dwOfs = DIK scancode, dwData = 0x80 for down / 0 for up, dwTimeStamp, dwSequence, uAppData). GetEvents then walks the batch in sequence order and, for every transition that matches a binding in the active input class, assigns the transition's level to that binding's flag. PollInput reads the flag; that is the turn axis, the walk axis, everything. Transitions that match no binding are skipped. Nothing re-reads the device.

Where releases go missing (both games, same code shape): * CExoRawInputInternal::SetActive 0->1 drains the DirectInput buffer (ClearKeyboardBuffer) without touching the flags. The engine drives that edge on window activate/deactivate, fullscreen toggle, area transitions, load screens, movies, dialogue start and end, the pause popup and the quit popup. KOTOR 2 logs prove it: the pause key's own release is lost on essentially every press (Diag.ClientHIE 218 down, never up). * GetKeyboardBuffer's own error path: DIERR_INPUTLOST / DIERR_NOTACQUIRED make it re-Acquire and return an EMPTY batch; whatever queued in the gap is gone. Foreground cooperative level, so any foreground blip does it. * Input-class switches also drain the buffer, but through ClearEvents, which zeroes every flag first — those release keys rather than strand them, and are not our concern.

Why a wrapper on the fetch, and not a hook on each discard site: the discard sites are many, one of them (the DirectInput-level loss) is invisible from outside, and the fix wanted is the same for all of them — make the engine's view of the keyboard converge on the device's. Doing it where the engine itself ingests transitions means the correction travels the engine's own path (sequence ordering, class matching, event emission) and no other code changes. KOTOR 2's joystick reader already works this way: it derives its transitions by diffing the device state each frame.

Safety rules, in priority order: 1. Never synthesise a press. A missed press is the safe failure (the key does nothing until pressed again); a synthesised one could fire an edge-triggered command twice if the real transition follows. 2. Synthesise a release only after two consecutive frames of disagreement with no transition for that key in this frame or the previous one. A real release that lands between the engine's buffer read and our state read shows up in the next batch and cancels the synthesis, so a duplicate is impossible; only a transition DirectInput itself never delivered survives the wait. 3. Gate on the reader's own gates (active, keyboard mode, device present). While the device is unacquired the true state is unknown; nothing is appended, and the next acquired frame catches up. 4. Any fault disables the resync for the session; the wrapper keeps forwarding to the engine's reader regardless.

C2712: everything under __try is POD and file-scope; no objects, no function-local statics.

## Declarations (in source order)

- L125 `void ResetShadow()`
- L133 `void Resync(void* rawInput, DeviceBuffer* buf, int readerRc)`: Everything after the engine's reader returned.
- L270 `int __fastcall GetKeyboardBufferResynced(void* rawInput, void* /*edx*/, DeviceBuffer* buf)`: Stand-in for the CALL inside GetEvents.
- L281 `void InstallKeyboardStreamResync()`
