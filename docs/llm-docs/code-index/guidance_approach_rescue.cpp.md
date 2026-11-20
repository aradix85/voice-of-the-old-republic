# guidance_approach_rescue.cpp (756 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L87 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L98 `bool UseStillQueued()`: A use still in the queue once the PC has stood still for the stall window.
- L106 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L125 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount)`: Living creatures (party included, the leader not) within `radius` of `pc`.
- L154 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed) and never back along an already-trie...
- L193 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L218 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L247 `bool ClosedDoorBy(const Vector& tgt, void* target, char* name, size_t nameSize)`
- L259 `bool SpeakClosedDoorBy(const Vector& tgt, void* target, const char* lead)`: Says "A closed Tomb Door stands by it." after `lead` (the refusal the door explains, or null when that has been spoken) when ClosedDoorBy finds one.
- L274 `bool RuleUnreachableTarget(const char* text)`: Feedback-router rule: the engine's own refusal of an attack or a use it cannot path to ("Unable to reach target!", Bash on a sarcophagus behind its door).
- L285 `void RegisterMsgRule()`
- L298 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L322 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L368 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L394 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L446 `bool StartRouteHop()`: Walks to the current hop of the route.
- L454 `void EndRoute(const char* why)`
- L462 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L506 `void LearnTurnback()`
- L547 `void RouteRoundTurnbacks()`
- L560 `void ForgetTurnbackResume()`
- L564 `void TickTurnbackResume()`
- L593 `bool NextRouteHop()`
- L605 `bool RetryRouteHop()`
- L616 `bool TryFloorNearestTarget(const Vector& tgt)`
- L645 `bool UseNeverLanded()`
- L652 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L673 `void OnBlocked(DWORD stalledMs)`
- L728 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
