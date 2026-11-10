# focus_guard_movie.cpp (70 lines)

Source: `patches/Accessibility/focus_guard_movie.cpp`

The movie-end foreground guard. A cutscene's movie handoff can leave no window at all owning the foreground (the K2 Peragus docking stunt); the engine auto-pauses on the focus loss and never wakes, which to a blind player is "Paused" and then silence. For a short time after each movie end the guard reclaims the render window, but only while the foreground is orphaned: a foreign owner is a deliberate alt-tab, our own owner means nothing is wrong, and a new movie segment must keep its focus (forcing it away mid-playback can abort the movie queue and close the game).

## Declarations (in source order)

- L32 `void Disarm(const char* why)`
- L39 `void TickMovieEndGuard(HWND liveRender)`
