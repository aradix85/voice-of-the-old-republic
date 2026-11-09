# intro_skip.h (41 lines)

Source: `patches/Accessibility/intro_skip.h`

Runtime-toggle helpers for the launch-time intro movies (biologo / leclogo / legal). Mirrors installer/IntroMovieDisabler.cs on the runtime side so the in-game mod-settings panel can flip the state after install.

State persistence is the filesystem itself — no separate config: * If `Movies/biologo.bik` exists → intros play on next launch. * If `Movies/biologo.bik.disabled` exists → intros are skipped. We only check biologo.bik as the representative; if intros were disabled by the installer all three (biologo / leclogo / legal) were renamed together, and the toggle keeps them in lockstep.

The rename takes effect on the NEXT launch — the engine plays movies only at startup, so a mid-session toggle won't affect the current run. Caller is expected to speak a "takes effect on next launch" cue after flipping.

## Declarations (in source order)

- L27 `enum class State`: Three-state report.
- L33 `State CurrentState()`
- L38 `bool SetDisabled(bool disable)`: Flip toward the requested target.
