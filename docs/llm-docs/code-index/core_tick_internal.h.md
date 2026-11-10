# core_tick_internal.h (27 lines)

Source: `patches/Accessibility/core_tick_internal.h`

Internal to the core_tick module (core_tick*.cpp); no other file includes it. The public interface is core_tick.h. Here: what the files share.

## Declarations (in source order)

- L16 `void EnsureSessionBringup()`: core_dllmain.cpp — the once-per-session stand-up (guards, speech, language, config snapshot, keymap, focus probe, bring-up nag).
- L23 `void RetryColdStartReacquire()`: core_tick_keyboard.cpp
- L24 `void WatchDeadKeyboard()`
