# combat_queue.cpp (366 lines)

Source: `patches/Accessibility/combat_queue.cpp`

The Shift+H combat queue overlay: every party member's queued actions as one list, with Enter to remove the focused one and Shift+Enter to clear all.

## Declarations (in source order)

- L30 `bool RemoveLastAction(void* combatRound)`
- L62 `void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize)`: A companion's name; the chargen name only guards an empty read.
- L72 `void AddRows(void* creature, void* round, int local, const char* name)`
- L84 `int BuildRows()`: Rebuild g_state.rows from the engine.
- L127 `void SpeakRow(int idx)`: Speak the focused row at index `idx` (0-based) of `count` total.
- L167 `bool RemoveRow(int idx)`: Removes row `idx` if it is the last of its creature's queue.
- L182 `bool ClearAllRows()`: Back to front, so every remove takes its creature's last action.
- L196 `bool IsActive()`
- L198 `void ForceDisarm(const char* reason)`
- L208 `bool Open()`
- L232 `void ClearAllAndClose()`
- L249 `bool HandleInputEvent(int code, int value)`
- L343 `void Tick()`
- L356 `void PollWin32Hotkey()`
