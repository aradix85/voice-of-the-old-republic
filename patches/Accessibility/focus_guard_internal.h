// Shared between the focus_guard files: the render window the poll thread
// tracks, the movie-end guard's arm state, and the two per-poll steps.

#pragma once

#include <windows.h>
#include <atomic>

#include "focus_guard.h"

namespace acc::focus_guard {

extern std::atomic<HWND> g_liveRenderWindow;  // newest visible "Render Window"

constexpr DWORD kMovieReclaimWindowMs = 2000;
extern std::atomic<DWORD> g_movieReclaimDeadline;  // GetTickCount() deadline; 0 = disarmed
extern std::atomic<int>   g_movieReclaims;

void MaybeReclaimAfterMovie();
void MaybeFlagInputBlockedWarning();

}  // namespace acc::focus_guard
