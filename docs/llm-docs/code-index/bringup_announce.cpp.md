# bringup_announce.cpp (302 lines)

Source: `patches/Accessibility/bringup_announce.cpp`

(From bringup_announce.h.) Loading-phase nag: while the engine is between "intros finished" and "main menu input pump live", arrow-key presses don't do anything because the engine is recreating Render Window instances and shuffling focus through them. Silent multi-second windows in this phase feel like a hang to a blind user.

This module spins up a polling thread that derives the bringup phase from the game's own windows, and if it detects an arrow/Enter/Space keypress during the Loading phase it speaks "Game is still loading" once. Silent during MoviesPlaying (audio would clash with intro audio) and during Responsive (no nag needed).

Detection is fully derived from the game's window state + the input-pump-live signal — no callbacks from diag_focus needed, so the two modules stay decoupled. Foreground class name identifies the Render Window; movie playback is keyed on the movie window EXISTING, because it is ghosted out of the foreground once it stops pumping (see IsMoviePlaying below).

## Declarations (in source order)

- L23 `const char* PhaseName(Phase p)`
- L68 `bool IsOurWindowOfClass(HWND hwnd, const char* targetClass)`: True iff `hwnd` belongs to the current process AND its class is the given target.
- L91 `bool IsLiveMovieWindow(HWND hwnd)`: A live, on-screen movie window of ours.
- L97 `BOOL CALLBACK FindMovieWindowProc(HWND hwnd, LPARAM lp)`
- L120 `bool MoviePlaying()`: True while an engine movie is on screen.
- L148 `void TransitionTo(Phase next, const char* reason)`
- L165 `Phase ClassifyForeground()`: Map current foreground-window class to a phase.
- L190 `bool UserNavKeyHeld()`
- L204 `DWORD WINAPI PollProc(LPVOID)`
- L263 `void Start()`
- L281 `void NotifyInputPumpLive()`
- L285 `bool IsInputPumpLive()`
- L289 `bool IsMoviePlaying()`
- L293 `bool MovieRecentlyPlaying(unsigned int windowMs)`
