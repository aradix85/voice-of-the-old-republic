# guidance_approach_rescue.cpp (706 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L85 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L96 `bool UseStillQueued()`: A use still in the queue once the PC has stood still for the stall window.
- L104 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L123 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount)`: Living creatures (party included, the leader not) within `radius` of `pc`.
- L152 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed) and never back along an already-trie...
- L191 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L216 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L249 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L273 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L319 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L345 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L397 `bool StartRouteHop()`: Walks to the current hop of the route.
- L405 `void EndRoute(const char* why)`
- L413 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L457 `void LearnTurnback()`
- L498 `void RouteRoundTurnbacks()`
- L511 `void ForgetTurnbackResume()`
- L515 `void TickTurnbackResume()`
- L544 `bool NextRouteHop()`
- L556 `bool RetryRouteHop()`
- L567 `bool TryFloorNearestTarget(const Vector& tgt)`
- L596 `bool UseNeverLanded()`
- L603 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L624 `void OnBlocked(DWORD stalledMs)`
- L678 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
