# focus_guard_engine_active.cpp (92 lines)

Source: `patches/Accessibility/focus_guard_engine_active.cpp`

The engine's own "render window active" flag, kept in step with the foreground (the maker's v0.9.3 fix, ported).

DeactivateRenderWindow (KOTOR 1 0x00401d90, KOTOR 2 0x0040BC40) pauses the world, turns the client's input off and clears the flag; ActivateRenderWindow sets it again and undoes both. They run from the window procedure on WM_ACTIVATEAPP. KOTOR 2's step after an area load hides the loading screen only while the flag is set, and its movie-end handler activates the window only in full screen. In a window, a stray WM_ACTIVATEAPP(0) at a movie's end therefore leaves the loading screen up for good: nothing sends WM_ACTIVATEAPP(1) again while the game keeps the foreground. The maker's tester got out with Alt+Tab away and back, which is what the call below does.

## Declarations (in source order)

- L51 `bool CallActivateRenderWindow()`
- L62 `void TickEngineActive()`
