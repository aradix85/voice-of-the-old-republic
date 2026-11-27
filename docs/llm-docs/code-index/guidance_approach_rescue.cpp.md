# guidance_approach_rescue.cpp (1015 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L76 `void KeepForResume(bool round)`: Keeps the target of the approach in flight to walk to again.
- L103 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L114 `bool UseStillQueued()`: A use still in the queue once the PC has stood still for the stall window.
- L122 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L143 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount, bool placeablesToo = false)`: Living creatures (party included, the leader not) within `radius` of `pc`, and with `placeablesToo` the placeables as well: what a step should keep clear of.
- L173 `bool StraightStepFree(const Vector& pc, const Vector& c)`: True when the PC can step from `pc` to `c` in a straight line: `c` lies over walkable floor and no wall stands between.
- L191 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed), never back along an already-tried d...
- L231 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L256 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L285 `bool IsClosedDoorOtherThan(void* door, void* target)`
- L290 `void* ClosedDoorBy(const Vector& tgt, void* target)`
- L301 `void* ClosedDoorBetween(const Vector& from, const Vector& tgt, void* target)`
- L320 `bool SpeakClosedDoor(void* door, const char* lead)`: Says "A closed Tomb Door stands by it." after `lead` (the refusal the door explains, or null when that has been spoken).
- L337 `bool RuleUnreachableTarget(const char* text)`: Feedback-router rule: the engine's own refusal of an attack or a use it cannot path to ("Unable to reach target!", Bash on a sarcophagus behind its door).
- L348 `void RegisterMsgRule()`
- L352 `bool SpeakClosedDoorBetween(const Vector& from, const Vector& tgt)`
- L365 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L395 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L449 `void ReleaseHandStep()`
- L457 `bool HandStepActive()`
- L461 `void TickHandStep()`
- L471 `bool TryHandStep(const Vector& tgt)`
- L549 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L575 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L627 `bool StartRouteHop()`: Walks to the current hop of the route.
- L635 `void EndRoute(const char* why)`
- L643 `bool TryRouteWalk(const Vector& tgt, const char* why)`
- L687 `void LearnTurnback()`
- L730 `void HoldBeforeDeadlyZone(const Vector& tgt)`: A deadly zone lies on the straight way and the mod knows no way round it, so this walk must not run.
- L765 `void RouteRoundTurnbacks()`
- L781 `void ForgetTurnbackResume()`
- L785 `void TickTurnbackResume()`
- L816 `bool ResumeAfterNoticeOnTheWay()`
- L850 `bool NextRouteHop()`
- L862 `bool RetryRouteHop()`
- L873 `bool TryFloorNearestTarget(const Vector& tgt)`
- L902 `bool UseNeverLanded()`
- L909 `bool TryRegionWalk()`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L930 `void OnBlocked(DWORD stalledMs)`
- L987 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
