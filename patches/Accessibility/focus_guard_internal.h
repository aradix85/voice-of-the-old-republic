// Shared between the focus_guard files: the two steps the poll thread in
// focus_guard.cpp runs every 100 ms, and the process-name lookup.

#pragma once

#include <windows.h>

#include "focus_guard.h"

namespace acc::focus_guard {

// The movie-end guard (focus_guard_movie.cpp). `liveRender` is the newest
// visible "Render Window", or null.
void TickMovieEndGuard(HWND liveRender);

// The Big Picture warning (focus_guard_bigpicture.cpp).
void MaybeFlagInputBlockedWarning();

// The bare exe name of process `pid` (written into `buf`), or nullptr when it
// cannot be read.
const char* ProcessExeName(DWORD pid, char* buf, DWORD bufSize);

}  // namespace acc::focus_guard
