# camera_announce.cpp (441 lines)

Source: `patches/Accessibility/camera_announce.cpp`

divides by an inter-tick interval, which GetTickCount cannot resolve (see the NowMs contract)

## Declarations (in source order)

- L98 `float AngularDelta(float a, float b)`
- L132 `bool ReadCameraCompass(float& outCompass)`
- L152 `void Tick()`
- L389 `bool AnnounceCurrentFacing(unsigned int dedupMs)`
- L431 `bool TryGetCameraEngineYawDegrees(float& out)`
