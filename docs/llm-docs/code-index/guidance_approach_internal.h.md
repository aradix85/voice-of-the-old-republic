# guidance_approach_internal.h (110 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L17 `struct ApproachState`
- L78 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L82 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L84 `void MarkWalkStart()`
- L87 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L91 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L94 `bool TickPartyHold(const Vector& pos, DWORD now)`: guidance_approach_gather.cpp.
- L97 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L98 `bool TrySidestep(const Vector& tgt)`
- L99 `void ResumeAfterSidestep()`
- L100 `bool TryReissueUse(float gap)`
- L106 `bool TryRegionWalk()`: The last rung before "way blocked": the engine does not walk it, but the region graph may know a way.
- L107 `void OnBlocked(DWORD stalledMs)`

Also 2 constants or variables at namespace scope.
