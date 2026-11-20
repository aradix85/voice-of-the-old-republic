# guidance_approach_rescue.cpp (793 lines)

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
- L247 `bool IsClosedDoorOtherThan(void* door, void* target)`
- L252 `void* ClosedDoorBy(const Vector& tgt, void* target)`
- L263 `void* ClosedDoorBetween(const Vector& from, const Vector& tgt, void* target)`
- L282 `bool SpeakClosedDoor(void* door, const char* lead)`: Says "A closed Tomb Door stands by it." after `lead` (the refusal the door explains, or null when that has been spoken).
- L299 `bool RuleUnreachableTarget(const char* text)`: Feedback-router rule: the engine's own refusal of an attack or a use it cannot path to ("Unable to reach target!", Bash on a sarcophagus behind its door).
- L310 `void RegisterMsgRule()`
- L314 `bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt)`
- L327 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L357 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L403 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L429 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L481 `bool StartRouteHop()`: Walks to the current hop of the route.
- L489 `void EndRoute(const char* why)`
- L497 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L541 `void LearnTurnback()`
- L582 `void RouteRoundTurnbacks()`
- L595 `void ForgetTurnbackResume()`
- L599 `void TickTurnbackResume()`
- L628 `bool NextRouteHop()`
- L640 `bool RetryRouteHop()`
- L651 `bool TryFloorNearestTarget(const Vector& tgt)`
- L680 `bool UseNeverLanded()`
- L687 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L708 `void OnBlocked(DWORD stalledMs)`
- L765 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
