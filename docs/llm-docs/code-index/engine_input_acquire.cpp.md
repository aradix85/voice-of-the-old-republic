# engine_input_acquire.cpp (209 lines)

Source: `patches/Accessibility/engine_input_acquire.cpp`

DirectInput device state: the change log, and the deferred reacquire and release of the keyboard and mouse.

## Declarations (in source order)

- L66 `bool ReadDiSnapshot(DiSnapshot* out)`
- L92 `void LogDirectInputStateIfChanged()`
- L129 `bool ForceReacquireInput()`
- L155 `bool ReleaseInput()`
- L186 `void RequestInputReacquire()`
- L190 `void RequestInputRelease()`
- L194 `void DrainPendingReacquire()`
