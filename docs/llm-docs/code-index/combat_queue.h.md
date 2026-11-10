# combat_queue.h (132 lines)

Source: `patches/Accessibility/combat_queue.h`

Combat action-queue submenu.

While active: Open        — speaks "Action queue, N actions." (or that it is empty). Up/Down     — cycles entries, "Attack Malak, 1 of 3". Enter       — removes focused entry; re-announces or closes if empty. Shift+Enter — clears all + closes. Esc         — closes.

Self-disarms when controlled creature changes / combat round drops / chain faults.

Skeleton: positional remove is unresolved (RemoveLastAction only removes the tail). Enter on the tail clears all; Enter on non-tail speaks QueueRemoveFailed. See combat-system "open" item for the positional-remove primitive search.

## Declarations (in source order)

- L27 `int CountPlayerEntries()`: The engine's own count of the controlled character's combat round, its leading placeholder included: the number AddAction's cap check looks at.
- L37 `void ReportPrePressDepth()`: Snapshot the queue depth RIGHT BEFORE the engine dispatches a press.
- L42 `int GetPrePressDepth()`: Returns the depth stashed by the most recent ReportPrePressDepth() call (or -1 if none).
- L51 `void ArmUserQueueAdd()`: Arm the user-attribution latch: "a user press is dispatching a queue add right now." Called from the two user-initiated queue sites — the bare-key dispatch (input_pipeline) and the unified-menu Enter.
- L73 `void ArmNoOpWatch()`: A user press that produces NO queue add — PERSONAL actions only.
- L78 `void CancelNoOpWatch()`: Cancel a pending watch when the caller has already explained the outcome — otherwise KOTOR 1's spoken refusal reason would be followed by the generic line saying the same thing twice.
- L82 `void TickNoOpWatch()`: Per-tick verdict for the watch above.
- L93 `void OnEngineActionAdded(void* combatRound, void* action)`: Announce a queued action the engine just accepted (or rejected at the cap) — the authoritative "X, position N" / "X, queue full" cue.
- L97 `bool Open()`: True iff armed (queue had >= 1 entry — an empty queue says so and returns false).
- L99 `bool IsActive()`
- L102 `bool HandleInputEvent(int code, int value)`: Press-edge only.
- L105 `void ForceDisarm(const char* reason)`: Engine owns the queue — no engine-side cleanup needed.
- L107 `void Tick()`
- L108 `void PollWin32Hotkey()`
- L113 `void TickIdleNotice()`: Speaks the idle notice (KOTOR 1): in combat, nothing queued for the controlled character and an enemy near for 2.5 s -> "You are not attacking..." once, until it attacks again or combat ends.
- L118 `bool LeaderIsAttacking(uint32_t targetId)`: True when the controlled character's first queued combat action targets `targetId` (client or server id; the high bit is ignored).
- L123 `int ReadRoundActionCount(void* combatRound)`: Queue depths for the diagnostics: the combat round's own count (0 also for a round without an action list, -1 when unreadable), the same for a creature's round, and the creature's outer action queue.
- L124 `int ReadQueueSize(void* serverCreature)`
- L125 `int ReadOuterQueueSize(void* serverCreature)`
- L129 `const char* RoleTag(void* combatRound)`: "PLAYER" when `combatRound` belongs to the user's creature, "other" for the companion and enemy rounds that share the same engine surfaces.
