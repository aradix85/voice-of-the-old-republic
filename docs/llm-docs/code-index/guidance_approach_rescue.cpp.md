# guidance_approach_rescue.cpp (443 lines)

Source: `patches/Accessibility/guidance_approach_rescue.cpp`

Rescuing a stalled approach: the coordinate retry, the sidestep round creatures in the way, the re-issued use, and the "way blocked" message.

## Declarations (in source order)

- L28 `Vector RescueDestination(const Vector& tgt)`: Where a rescue walk should go.
- L47 `int NearbyCreatures(const Vector& pc, float radius, Vector* outPos, void** outObj, int maxCount)`: Living creatures (party included, the leader not) within `radius` of `pc`.
- L76 `bool ChooseSidestep(const Vector& pc, const Vector& tgt, Vector& out, float& outYaw, float& outNearest)`: Pick a sidestep point kSidestepDist from `pc`: as far from nearby creatures as possible, leaning toward the target, but never straight at it (that line is the one that just failed) and never back along an already-trie...
- L116 `bool CreatureInTheWay(char* name, size_t nameSize)`: Nearest living creature within kInTheWayRadius of the PC, for the final blocked line.
- L142 `bool CompanionInTheWay(const Vector& tgt, char* name, size_t nameSize)`: A party member standing at the target, closer to it than the player and within a few metres: the walk cannot get past them to the use point.
- L175 `bool TryCoordinateWalkRetry(const Vector& tgt)`: The walk never physically started (see the header note): re-issue it as a plain coordinate walk to the target position rather than hand the player a "way blocked" they would reasonably read as "there is no route at all".
- L212 `bool TrySidestep(const Vector& tgt)`: Rooted even after the coordinate retry: step aside and walk again (see kSidestepMax).
- L265 `void ResumeAfterSidestep()`: The sidestep landed (or got partway): walk to the target again.
- L295 `bool TryReissueUse(float gap)`: The coordinate-walk retry only walks.
- L350 `void OnBlocked(DWORD stalledMs)`: Break a blocked approach: cancel the bouncing walk, restore input if the caller disabled it (explicit, so engine_player's queue-watched restore and this tracker never fight), clear dialog-pending limbo (talk only), an...
- L410 `void SpeakWayBlocked(const char* name, const Vector& targetPos)`
