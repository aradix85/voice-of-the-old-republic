# core_tick_internal.h (15 lines)

Source: `patches/Accessibility/core_tick_internal.h`

Internal to the core_tick module (core_tick*.cpp); no other file includes it. The public interface is core_tick.h.

## Declarations (in source order)

- L11 `void RetryColdStartReacquire()`: core_tick_keyboard.cpp, called once per tick before any input poll.
- L12 `void WatchDeadKeyboard()`
