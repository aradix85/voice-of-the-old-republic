# guidance_approach_internal.h (162 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L20 `struct ApproachState`
- L94 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L98 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L100 `void MarkWalkStart()`
- L103 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L107 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L110 `bool TickPartyHold(const Vector& pos, DWORD now)`: guidance_approach_gather.cpp.
- L113 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L114 `bool TrySidestep(const Vector& tgt)`
- L115 `void ResumeAfterSidestep()`
- L116 `bool TryReissueUse(float gap)`
- L122 `bool TryFloorNearestTarget(const Vector& tgt)`: The PC has settled and the use it was sent to do is still queued: the engine cannot get to the use point from this side (a terminal on a desk that only the far side comes close to).
- L125 `bool UseNeverLanded()`: True when that walk and its re-fired use still left the use queued: the use point cannot be reached, and saying so beats a silent disarm.
- L132 `bool TryRouteWalk(const Vector& tgt, const char* why)`: The mod walks its own path over the nav graph, hop by hop of a few metres; the arrival acts as after the coordinate retry.
- L138 `void LearnTurnback()`: Turn-back zones (turnback_zones.h).
- L141 `void RouteRoundTurnbacks()`: The fresh approach would walk straight into a known zone: walks the mod's own path round it instead.
- L143 `void ForgetTurnbackResume()`: Drops the target kept by LearnTurnback.
- L146 `void TickTurnbackResume()`: With no approach in flight: once the message has closed and the party has been put back, repeats the Enter that was turned back.
- L149 `bool NextRouteHop()`: The hop was reached: starts the next one.
- L152 `bool RetryRouteHop()`: A hop stalled short of its point: starts it again, twice at most.
- L158 `bool TryRegionWalk()`: The last rung before "way blocked": the engine does not walk it, but the region graph may know a way.
- L159 `void OnBlocked(DWORD stalledMs)`

Also 3 constants or variables at namespace scope.
