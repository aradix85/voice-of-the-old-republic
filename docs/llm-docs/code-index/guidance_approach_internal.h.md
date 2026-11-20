# guidance_approach_internal.h (181 lines)

Source: `patches/Accessibility/guidance_approach_internal.h`

Internal to the guidance_approach module (guidance_approach*.cpp); no other file includes it. The public interface is guidance_approach.h.

## Declarations (in source order)

- L20 `struct ApproachState`
- L99 `void EndApproach(ApproachOutcome outcome)`: guidance_approach.cpp
- L103 `bool ResolveTargetPos(Vector& out)`: The live target position when the object is readable, else the stamped fallback; false when neither is available.
- L105 `void MarkWalkStart()`
- L108 `void RestartStallWindow()`: Gives a fresh walk or use a full stall window before the next verdict.
- L112 `void ReleaseApproach()`: Ends the engine side of an approach: cancels the walk, hands input back when the arm took it, and clears the dialog-pending state of a talk.
- L115 `bool TickPartyHold(const Vector& pos, DWORD now)`: guidance_approach_gather.cpp.
- L118 `bool TryCoordinateWalkRetry(const Vector& tgt)`: guidance_approach_rescue.cpp
- L119 `bool TrySidestep(const Vector& tgt)`
- L120 `void ResumeAfterSidestep()`
- L126 `bool TryHandStep(const Vector& tgt)`: Rooted where the engine starts no walk at all (inside the footprint of a container, between people): holds the player's own movement key for a moment, toward the freest of forward, back, left and right, as a player wo...
- L128 `void TickHandStep()`: Every tick, armed or not: lets that key go when its time is up.
- L129 `bool TryReissueUse(float gap)`
- L135 `bool TryFloorNearestTarget(const Vector& tgt)`: The PC has settled and the use it was sent to do is still queued: the engine cannot get to the use point from this side (a terminal on a desk that only the far side comes close to).
- L138 `bool UseNeverLanded()`: True when that walk and its re-fired use still left the use queued: the use point cannot be reached, and saying so beats a silent disarm.
- L145 `bool TryRouteWalk(const Vector& tgt, const char* why)`: The mod walks its own path over the nav graph, hop by hop of a few metres; the arrival acts as after the coordinate retry.
- L151 `void LearnTurnback()`: Turn-back zones (turnback_zones.h).
- L154 `void RouteRoundTurnbacks()`: The fresh approach would walk straight into a known zone: walks the mod's own path round it instead.
- L156 `void ForgetTurnbackResume()`: Drops the target kept by LearnTurnback.
- L159 `void TickTurnbackResume()`: With no approach in flight: once the message has closed and the party has been put back, repeats the Enter that was turned back.
- L162 `bool NextRouteHop()`: The hop was reached: starts the next one.
- L165 `bool RetryRouteHop()`: A hop stalled short of its point: starts it again, twice at most.
- L171 `bool TryRegionWalk()`: The last rung before "way blocked": the engine does not walk it, but the region graph may know a way.
- L172 `void OnBlocked(DWORD stalledMs)`
- L175 `void TellWhoStayedBehind()`: A walk arrived: names the companions that are far behind (guidance_approach_gather.cpp).
- L178 `bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt)`: The approach ended at the target without its act: says "A closed Door stands by it." when a closed door is on the line from `from` to the target.

Also 3 constants or variables at namespace scope.
