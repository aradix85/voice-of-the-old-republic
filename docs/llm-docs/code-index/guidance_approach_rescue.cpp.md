# guidance_approach_rescue.cpp (838 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L88 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L99 `bool UseStillQueued()`: A use still in the queue once the PC has stood still for the stall window.
- L107 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L126 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount)`: Living creatures (party included, the leader not) within `radius` of `pc`.
- L155 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed) and never back along an already-trie...
- L194 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L219 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L248 `bool IsClosedDoorOtherThan(void* door, void* target)`
- L253 `void* ClosedDoorBy(const Vector& tgt, void* target)`
- L264 `void* ClosedDoorBetween(const Vector& from, const Vector& tgt, void* target)`
- L283 `bool SpeakClosedDoor(void* door, const char* lead)`: Says "A closed Tomb Door stands by it." after `lead` (the refusal the door explains, or null when that has been spoken).
- L300 `bool RuleUnreachableTarget(const char* text)`: Feedback-router rule: the engine's own refusal of an attack or a use it cannot path to ("Unable to reach target!", Bash on a sarcophagus behind its door).
- L311 `void RegisterMsgRule()`
- L315 `bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt)`
- L328 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L358 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L404 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L430 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L482 `bool StartRouteHop()`: Walks to the current hop of the route.
- L490 `void EndRoute(const char* why)`
- L498 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L542 `void LearnTurnback()`
- L589 `void HoldBeforeDeadlyZone(const Vector& tgt)`: A deadly zone lies on the straight way and the mod knows no way round it, so this walk must not run.
- L624 `void RouteRoundTurnbacks()`
- L640 `void ForgetTurnbackResume()`
- L644 `void TickTurnbackResume()`
- L673 `bool NextRouteHop()`
- L685 `bool RetryRouteHop()`
- L696 `bool TryFloorNearestTarget(const Vector& tgt)`
- L725 `bool UseNeverLanded()`
- L732 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L753 `void OnBlocked(DWORD stalledMs)`
- L810 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
