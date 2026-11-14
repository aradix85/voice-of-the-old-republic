# guidance_approach_internal.h (122 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L17 `struct ApproachState`
- L81 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L85 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L87 `void MarkWalkStart()`
- L90 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L94 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L97 `bool TickPartyHold(const Vector& pos, DWORD now)`: guidance_approach_gather.cpp.
- L100 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L101 `bool TrySidestep(const Vector& tgt)`
- L102 `void ResumeAfterSidestep()`
- L103 `bool TryReissueUse(float gap)`
- L109 `bool TryFloorNearestTarget(const Vector& tgt)`: The PC has settled and the use it was sent to do is still queued: the engine cannot get to the use point from this side (a terminal on a desk that only the far side comes close to).
- L112 `bool UseNeverLanded()`: True when that walk and its re-fired use still left the use queued: the use point cannot be reached, and saying so beats a silent disarm.
- L118 `bool TryRegionWalk()`: The last rung before "way blocked": the engine does not walk it, but the region graph may know a way.
- L119 `void OnBlocked(DWORD stalledMs)`

Also 2 constants or variables at namespace scope.
