# engine_input_acquire.cpp (186 lines)

Source: `patches/Accessibility/engine_input_acquire.cpp`

DirectInput device state: the change log, and the deferred reacquire and release of the keyboard and mouse.

## Declarations (in source order)

- L56 `bool ReadDiSnapshot(DiSnapshot* out)`
- L79 `bool SameDi(const DiSnapshot& a, const DiSnapshot& b)`
- L86 `bool CallSetActive(const int* states, int count, void** exoOut)`: SetActive with each of `states` in turn; *exoOut gets the facade.
- L101 `void LogDirectInputStateIfChanged()`
- L131 `bool ForceReacquireInput()`
- L146 `bool ReleaseInput()`
- L163 `void RequestInputReacquire()`
- L167 `void RequestInputRelease()`
- L171 `void DrainPendingReacquire()`
