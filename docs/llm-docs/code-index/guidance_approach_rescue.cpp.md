# guidance_approach_rescue.cpp (440 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L48 `void StopForRescueWalk()`: Clears the failed plan and hands input back: WalkTo manages its own AI level and needs manual input left enabled.
- L60 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L79 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount)`: Living creatures (party included, the leader not) within `radius` of `pc`.
- L108 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed) and never back along an already-trie...
- L148 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L173 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point (Trask on the Endar Spire bridge door).
- L206 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L230 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L276 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L302 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks: it clears the failed plan to free the pathfinder, so the use it replaced is gone by the time the PC arrives.
- L353 `void OnBlocked(DWORD stalledMs)`: Breaks a blocked approach and announces "way blocked" when the arm asked for it, naming a companion at the target or a creature beside the PC when one is the reason.
- L407 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
