# guidance_approach_internal.h (126 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h. Here: what the files share.

## Declarations (in source order)

- L61 `struct ApproachState`
- L113 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L114 `bool ResolveTargetPos(Vector& out)`
- L115 `float GapTo(const Vector& tgt, const Vector& pos)`
- L116 `void MarkWalkStart()`
- L119 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L120 `bool TrySidestep(const Vector& tgt)`
- L121 `void ResumeAfterSidestep()`
- L122 `bool TryReissueUse(float gap)`
- L123 `void OnBlocked(DWORD stalledMs)`

Also 20 constants or variables at namespace scope.
