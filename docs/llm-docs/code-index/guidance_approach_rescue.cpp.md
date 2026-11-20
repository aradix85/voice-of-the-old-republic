# guidance_approach_rescue.cpp (971 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L91 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L102 `bool UseStillQueued()`: A use still in the queue once the PC has stood still for the stall window.
- L110 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L131 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount, bool placeablesToo = false)`: Living creatures (party included, the leader not) within `radius` of `pc`, and with `placeablesToo` the placeables as well: what a step should keep clear of.
- L161 `bool StraightStepFree(const Vector& pc, const Vector& c)`: True when the PC can step from `pc` to `c` in a straight line: `c` lies over walkable floor and no wall stands between.
- L179 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed), never back along an already-tried d...
- L219 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L244 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L273 `bool IsClosedDoorOtherThan(void* door, void* target)`
- L278 `void* ClosedDoorBy(const Vector& tgt, void* target)`
- L289 `void* ClosedDoorBetween(const Vector& from, const Vector& tgt, void* target)`
- L308 `bool SpeakClosedDoor(void* door, const char* lead)`: Says "A closed Tomb Door stands by it." after `lead` (the refusal the door explains, or null when that has been spoken).
- L325 `bool RuleUnreachableTarget(const char* text)`: Feedback-router rule: the engine's own refusal of an attack or a use it cannot path to ("Unable to reach target!", Bash on a sarcophagus behind its door).
- L336 `void RegisterMsgRule()`
- L340 `bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt)`
- L353 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L383 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L437 `void ReleaseHandStep()`
- L445 `bool HandStepActive()`
- L449 `void TickHandStep()`
- L459 `bool TryHandStep(const Vector& tgt)`
- L537 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L563 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L615 `bool StartRouteHop()`: Walks to the current hop of the route.
- L623 `void EndRoute(const char* why)`
- L631 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L675 `void LearnTurnback()`
- L722 `void HoldBeforeDeadlyZone(const Vector& tgt)`: A deadly zone lies on the straight way and the mod knows no way round it, so this walk must not run.
- L757 `void RouteRoundTurnbacks()`
- L773 `void ForgetTurnbackResume()`
- L777 `void TickTurnbackResume()`
- L806 `bool NextRouteHop()`
- L818 `bool RetryRouteHop()`
- L829 `bool TryFloorNearestTarget(const Vector& tgt)`
- L858 `bool UseNeverLanded()`
- L865 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L886 `void OnBlocked(DWORD stalledMs)`
- L943 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
