# guidance_approach.cpp (384 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

The unified walk-to-act approach tracker: arming it, the per-tick verdict (a conversation, panel or bark proved the act; the walk arrived; it stalled), cancelling it, and the queries.

## Declarations (in source order)

- L47 `bool IsTalkOrUseTarget(void* obj)`: Someone to talk to or something to use.
- L59 `void EndApproach(ApproachOutcome outcome)`
- L64 `bool ResolveTargetPos(Vector& out)`
- L76 `void MarkWalkStart()`
- L82 `void RestartStallWindow()`
- L87 `void ReleaseApproach()`
- L97 `void ArmApproach(const ApproachArm& arm)`
- L148 `void TickApproach()`
- L347 `bool IsApproachInFlight()`
- L351 `void CancelApproach()`
- L355 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L360 `void* ApproachTarget()`
- L364 `bool IsAnyModApproachInFlight()`
- L368 `bool CancelByMovement()`
