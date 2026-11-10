# combat_queue.cpp (407 lines)

Source: `patches/Accessibility/combat_queue.cpp`

GetObjectDisplayNameByHandle

## Declarations (in source order)

- L47 `void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize)`: Resolve the display name for a companion party member by handle.
- L59 `int BuildRows()`: Rebuild g_state.rows from live engine state.
- L132 `void SpeakRow(int idx)`: Speak the focused row at index `idx` (0-based) of `count` total.
- L182 `bool RemoveRow(int idx)`: Try to remove the action at row `idx`.
- L209 `bool ClearAllRows()`: Clear every party member's queue via repeated RemoveLastAction calls against each row's combat round.
- L231 `bool IsActive()`
- L233 `void ForceDisarm(const char* reason)`
- L243 `bool Open()`
- L267 `void ClearAllAndClose()`
- L284 `bool HandleInputEvent(int code, int value)`
- L382 `void Tick()`
- L396 `void PollWin32Hotkey()`
