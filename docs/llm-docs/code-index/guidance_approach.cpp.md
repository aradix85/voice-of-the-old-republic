# guidance_approach.cpp (348 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

The unified walk-to-act approach tracker: arming it, the per-tick verdict (a conversation, panel or bark proved the act; the walk arrived; it stalled), cancelling it, and the queries.

## Declarations (in source order)

- L47 `void EndApproach(ApproachOutcome outcome)`
- L52 `bool ResolveTargetPos(Vector& out)`
- L64 `void MarkWalkStart()`
- L70 `void RestartStallWindow()`
- L75 `void ReleaseApproach()`
- L85 `void ArmApproach(const ApproachArm& arm)`
- L135 `void TickApproach()`
- L311 `bool IsApproachInFlight()`
- L315 `void CancelApproach()`
- L319 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L324 `void* ApproachTarget()`
- L328 `bool IsAnyModApproachInFlight()`
- L332 `bool CancelByMovement()`
