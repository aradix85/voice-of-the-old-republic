# camera_announce.cpp (436 lines)

Source: `patches/Accessibility/camera_announce.cpp`

divides by an inter-tick interval, which GetTickCount cannot resolve (see the NowMs contract)

## Declarations (in source order)

- L96 `float AngularDelta(float a, float b)`
- L130 `bool ReadCameraCompass(float& outCompass)`
- L147 `void Tick()`
- L384 `bool AnnounceCurrentFacing(unsigned int dedupMs)`
- L426 `bool TryGetCameraEngineYawDegrees(float& out)`
