# guidance_approach.cpp (397 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

The unified walk-to-act approach tracker: arming it, the per-tick verdict (a conversation, panel or bark proved the act; the walk arrived; it stalled), cancelling it, and the queries.

## Declarations (in source order)

- L47 `bool IsTalkOrUseTarget(void* obj)`: Someone to talk to or something to use.
- L59 `void EndApproach(ApproachOutcome outcome)`
- L67 `bool ResolveTargetPos(Vector& out)`
- L79 `void MarkWalkStart()`
- L85 `void RestartStallWindow()`
- L90 `void ReleaseApproach()`
- L100 `void ArmApproach(const ApproachArm& arm)`
- L151 `void TickApproach()`
- L360 `bool IsApproachInFlight()`
- L364 `void CancelApproach()`
- L368 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L373 `void* ApproachTarget()`
- L377 `bool IsAnyModApproachInFlight()`
- L381 `bool CancelByMovement()`
