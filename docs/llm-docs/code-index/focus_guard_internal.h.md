# focus_guard_internal.h (24 lines)

Source: `patches/Accessibility/focus_guard_internal.h`

Shared between the focus_guard files: the two steps the poll thread in focus_guard.cpp runs every 100 ms, and the process-name lookup.

## Declarations (in source order)

- L14 `void TickMovieEndGuard(HWND liveRender)`: The movie-end guard (focus_guard_movie.cpp).
- L17 `void MaybeFlagInputBlockedWarning()`: The Big Picture warning (focus_guard_bigpicture.cpp).
- L21 `const char* ProcessExeName(DWORD pid, char* buf, DWORD bufSize)`: The bare exe name of process `pid` (written into `buf`), or nullptr when it cannot be read.
