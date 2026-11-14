# guidance_approach_internal.h (146 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L19 `struct ApproachState`
- L93 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L97 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L99 `void MarkWalkStart()`
- L102 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L106 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L109 `bool TickPartyHold(const Vector& pos, DWORD now)`: guidance_approach_gather.cpp.
- L112 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L113 `bool TrySidestep(const Vector& tgt)`
- L114 `void ResumeAfterSidestep()`
- L115 `bool TryReissueUse(float gap)`
- L121 `bool TryFloorNearestTarget(const Vector& tgt)`: The PC has settled and the use it was sent to do is still queued: the engine cannot get to the use point from this side (a terminal on a desk that only the far side comes close to).
- L124 `bool UseNeverLanded()`: True when that walk and its re-fired use still left the use queued: the use point cannot be reached, and saying so beats a silent disarm.
- L130 `bool TryRouteWalk(const Vector& tgt)`: The engine starts no walk to the target, though short walks go: it plots no path of its own there (the Ebon Hawk's corridors, Davik's hangar after the fight).
- L133 `bool NextRouteHop()`: The hop was reached: starts the next one.
- L136 `bool RetryRouteHop()`: A hop stalled short of its point: starts it again, twice at most.
- L142 `bool TryRegionWalk()`: The last rung before "way blocked": the engine does not walk it, but the region graph may know a way.
- L143 `void OnBlocked(DWORD stalledMs)`

Also 3 constants or variables at namespace scope.
