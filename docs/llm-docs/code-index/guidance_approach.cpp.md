# guidance_approach.cpp (316 lines)

Source: `patches/Accessibility/guidance_approach.cpp`

(From guidance_approach.h.) Unified approach tracker — the single "I dispatched a walk-to-act, watch whether it arrives, settles, or stalls" state machine.

Replaces three previously-independent observers that all keyed on the same signal (player movement after a dispatch): - guidance_autowalk's in-flight tracker + walk-blocked one-shot (Shift+-), - interact_hotkey's g_approach dialog/use watchdog (Enter), - cycle_input's autowalk block-watch (Shift+- announce wrapper).

One arm, one tick, one set of thresholds. Whoever dispatches a walk (Shift+- autowalk, Enter loot/talk/door/...) arms it with the target identity + the lifecycle flags; TickApproach() drives it: - SUCCESS  → a conversation opens (talk), an interaction panel opens (loot), or the PC settles within reach of the target. Disarm quietly. - BLOCKED  → the PC stalls (no movement for the stall window) while still far from the target (wedged against geometry / no walkable point in range). Cancel the bouncing walk, restore input if the caller disabled it, clear dialog-pending state (talk only), and — if the arm asked for it — announce "way blocked" with the target's live distance + direction.

Liveness is MOVEMENT, never the action-queue depth: the engine drains its queue to 0 transiently between walkmesh waypoints and UseObject's composite queue never drains, so a depth read mistakes live walks for finished ones.

That same movement signal also sets how much distance "settled within reach" forgives. A walk that ran and wedged gets six metres of slack; a walk that never took a step gets only use range, because at a standstill a small gap can only mean "we were already at it". Six metres there swallowed nine consecutive failed console interactions whole (patch-20260816-095834.log) — each one was inside the radius, so it disarmed silently and the retry below never ran.

That same movement signal separates the two verdicts a stall can carry. A walk that started and then wedged has moved at least once; a walk that never started has not moved at all, and for that case "way blocked" is a lie — the player hears "there is no route" when the route is fine and only the dispatch failed. AddUseObjectAction produces both shapes and reports success for each: it discards the action a frame later when it cannot resolve a use-node (queue 0→1→0 in ~50ms), or it drives a re-plan loop that churns the queue for the whole stall window (4↔6, PC rooted). So on a stall with no movement since the arm, the tracker clears the failed plan and retries once as a plain coordinate walk — the engine's coordinate A* routes over distances the use action refuses — and only announces "way blocked" if that stalls too.

Input restore on SUCCESS stays with engine_player's queue-watched session (TickPlayerInputRestore) — it knows the queue state. This tracker only force-restores on the BLOCKED path, where it is actively intervening, so the two never fight.

## Declarations (in source order)

- L25 `void EndApproach(ApproachOutcome outcome)`
- L32 `bool ResolveTargetPos(Vector& out)`: Live target position when the object is readable, else the stamped fallback.
- L45 `float GapTo(const Vector& tgt, const Vector& pos)`: Planar player→target gap, in metres.
- L49 `void MarkWalkStart()`
- L55 `void ArmApproach(const ApproachArm& arm)`
- L99 `void TickApproach()`
- L270 `bool IsApproachInFlight()`
- L274 `void CancelApproach()`
- L278 `ApproachOutcome LastApproachOutcome(uint32_t* serial)`
- L283 `void* ApproachTarget()`
- L287 `bool IsAnyModApproachInFlight()`
- L291 `bool CancelByMovement()`
