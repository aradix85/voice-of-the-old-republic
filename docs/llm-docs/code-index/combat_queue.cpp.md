# combat_queue.cpp (367 lines)

Source: `patches/Accessibility/combat_queue.cpp`

The Shift+H combat queue overlay: every party member's queued actions as one list, with Enter to remove the focused one and Shift+Enter to clear all.

## Declarations (in source order)

- L30 `bool RemoveLastAction(void* combatRound)`
- L62 `void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize)`: A companion's name; the chargen name only guards an empty read.
- L72 `void AddRows(void* creature, void* round, int local, const char* name)`
- L85 `int BuildRows()`: Rebuild g_state.rows from the engine.
- L128 `void SpeakRow(int idx)`: Speak the focused row at index `idx` (0-based) of `count` total.
- L168 `bool RemoveRow(int idx)`: Removes row `idx` if it is the last of its creature's queue.
- L183 `bool ClearAllRows()`: Back to front, so every remove takes its creature's last action.
- L197 `bool IsActive()`
- L199 `void ForceDisarm(const char* reason)`
- L209 `bool Open()`
- L233 `void ClearAllAndClose()`
- L250 `bool HandleInputEvent(int code, int value)`
- L344 `void Tick()`
- L357 `void PollWin32Hotkey()`
