# focus_guard.h (39 lines)

Source: `patches/Accessibility/focus_guard.h`

Diagnostics for the "menu loaded but unresponsive" / "intro movie plays twice" / "alt-tab needed to wake the engine" class of reports.

StartFocusProbe spins up a background polling thread that periodically re-enumerates the process's top-level windows and subclasses every game window it hasn't seen before. Subclassed WindowProc logs every WM_ACTIVATE / WM_ACTIVATEAPP / WM_SETFOCUS / WM_KILLFOCUS with the source HWND + class name tag. Polls every 100ms so transient windows (SWMovieWindow recreated between intros) get caught.

Idempotent — repeat calls are no-ops. Safe to call from OnRulesInit (early, before windows exist) and from MainMenu first-sight (late, belt-and-braces) without worrying about double-subclassing.

## Declarations (in source order)

- L21 `void StartFocusProbe()`: Start the focus-probe polling thread.
- L28 `bool GameOwnsForeground()`: True when the foreground window belongs to our process — i.e.
- L36 `void DrainInputBlockedWarning()`: Drain a pending "input blocked by Steam Big Picture" warning and speak it (no-op if none queued).
