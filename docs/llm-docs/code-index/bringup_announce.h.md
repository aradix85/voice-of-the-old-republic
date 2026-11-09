# bringup_announce.h (75 lines)

Source: `patches/Accessibility/bringup_announce.h`

Loading-phase nag: while the engine is between "intros finished" and "main menu input pump live", arrow-key presses don't do anything because the engine is recreating Render Window instances and shuffling focus through them. Silent multi-second windows in this phase feel like a hang to a blind user.

This module spins up a polling thread that derives the bringup phase from the game's own windows, and if it detects an arrow/Enter/Space keypress during the Loading phase it speaks "Game is still loading" once. Silent during MoviesPlaying (audio would clash with intro audio) and during Responsive (no nag needed).

Detection is fully derived from the game's window state + the input-pump-live signal — no callbacks from diag_focus needed, so the two modules stay decoupled. Foreground class name identifies the Render Window; movie playback is keyed on the movie window EXISTING, because it is ghosted out of the foreground once it stops pumping (see IsMoviePlaying below).

## Declarations (in source order)

- L26 `void Start()`: Spin up the polling thread.
- L31 `void NotifyInputPumpLive()`: Called from menus.cpp the moment we detect the user's first arrow-key nav on the MainMenu panel (= engine input pump is live).
- L42 `bool IsInputPumpLive()`: True once NotifyInputPumpLive has fired — i.e.
- L62 `bool IsMoviePlaying()`: True iff an engine movie is on screen — i.e.
- L72 `bool MovieRecentlyPlaying(unsigned int windowMs)`: True while a movie is on screen, and for `windowMs` after the most recent one disappears.
