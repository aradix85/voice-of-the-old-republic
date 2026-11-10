# guidance_approach_internal.h (92 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L17 `struct ApproachState`
- L69 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L73 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L75 `void MarkWalkStart()`
- L78 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L82 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L85 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L86 `bool TrySidestep(const Vector& tgt)`
- L87 `void ResumeAfterSidestep()`
- L88 `bool TryReissueUse(float gap)`
- L89 `void OnBlocked(DWORD stalledMs)`

Also 2 constants or variables at namespace scope.
