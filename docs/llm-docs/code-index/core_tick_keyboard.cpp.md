# core_tick_keyboard.cpp (389 lines)

Source: `patches/Accessibility/core_tick_keyboard.cpp`

Keeping the engine's DirectInput keyboard alive: the cold-start reacquire retry and the mid-session dead-keyboard watchdog.

## Declarations (in source order)

- L180 `int HeldEngineBoundKey()`: Held-key probe — shared by both reacquire paths below Engine-bound keys a player actually presses when nothing is responding.
- L190 `void SampleWorldResponse(ULONGLONG now)`
- L229 `void RetryColdStartReacquire()`
- L280 `void WatchDeadKeyboard()`
