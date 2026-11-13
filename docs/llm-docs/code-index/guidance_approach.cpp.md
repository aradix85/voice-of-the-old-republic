# guidance_approach.cpp (311 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

The unified walk-to-act approach tracker: arming it, the per-tick verdict (a conversation, panel or bark proved the act; the walk arrived; it stalled), cancelling it, and the queries.

## Declarations (in source order)

- L45 `void EndApproach(ApproachOutcome outcome)`
- L50 `bool ResolveTargetPos(Vector& out)`
- L62 `void MarkWalkStart()`
- L68 `void RestartStallWindow()`
- L73 `void ReleaseApproach()`
- L83 `void ArmApproach(const ApproachArm& arm)`
- L128 `void TickApproach()`
- L274 `bool IsApproachInFlight()`
- L278 `void CancelApproach()`
- L282 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L287 `void* ApproachTarget()`
- L291 `bool IsAnyModApproachInFlight()`
- L295 `bool CancelByMovement()`
