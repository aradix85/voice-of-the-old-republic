# core_tick_keyboard.cpp (265 lines)

Source: `patches/Accessibility/core_tick_keyboard.cpp`

Keeping the engine's DirectInput keyboard alive: the cold-start reacquire retry and the mid-session dead-keyboard watchdog.

Both recover by driving CExoInput::SetActive 0 -> 1 (ForceReacquireInput). The 1 -> 0 leg Unacquire()s the keyboard, mouse and joysticks; the 0 -> 1 leg Acquire()s and then drains the DirectInput buffers (ClearKeyboardBuffer, ClearMouseBuffer). The engine reads keys buffered, as a stream of transitions rather than as level state, so a key-up drained that way is lost for good and the engine holds that key down forever. Hence the rule both paths keep: never cycle while a key is held, and let a release settle first.

## Declarations (in source order)

- L34 `int HeldEngineBoundKey()`: The engine-bound key held right now, or 0: GUI confirm, cancel and nav, leader cycle, movement.
- L81 `void SampleWorldResponse(ULONGLONG now)`
- L120 `void RetryColdStartReacquire()`
- L169 `void WatchDeadKeyboard()`
