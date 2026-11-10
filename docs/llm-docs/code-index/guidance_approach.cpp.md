# guidance_approach.cpp (308 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

The unified walk-to-act approach tracker: arming it, the per-tick verdict (a conversation, panel or bark proved the act; the walk arrived; it stalled), cancelling it, and the queries.

## Declarations (in source order)

- L45 `void EndApproach(ApproachOutcome outcome)`
- L50 `bool ResolveTargetPos(Vector& out)`
- L62 `void MarkWalkStart()`
- L68 `void RestartStallWindow()`
- L73 `void ReleaseApproach()`
- L83 `void ArmApproach(const ApproachArm& arm)`
- L127 `void TickApproach()`
- L271 `bool IsApproachInFlight()`
- L275 `void CancelApproach()`
- L279 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L284 `void* ApproachTarget()`
- L288 `bool IsAnyModApproachInFlight()`
- L292 `bool CancelByMovement()`
