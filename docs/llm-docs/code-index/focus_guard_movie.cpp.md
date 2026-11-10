# focus_guard_movie.cpp (116 lines)

Source: `patches/Accessibility/focus_guard_movie.cpp`

The movie-end foreground guard: after a cutscene movie the foreground can end up owned by no window at all, which leaves the engine paused for good; the guard reclaims the render window then, and only then.

## Declarations (in source order)

- L21 `void ForceForeground(HWND target, HWND fg)`: Pull `target` to the foreground from a thread that doesn't own it.
- L74 `void MaybeReclaimAfterMovie()`
