# combat_queue.h (110 lines)

Source: `patches/Accessibility/combat_queue.h`

Combat action-queue submenu.

While active: Open        — speaks "Aktionsschlange, N Aktionen" (or "leer"). Up/Down     — cycles entries, "Angriff Malak, 1 von 3". Enter       — removes focused entry; re-announces or closes if empty. Shift+Enter — clears all + closes. Esc         — closes.

Self-disarms when controlled creature changes / combat round drops / chain faults.

Skeleton: positional remove is unresolved (RemoveLastAction only removes the tail). Enter on the tail clears all; Enter on non-tail speaks QueueRemoveFailed. See combat-system "open" item for the positional-remove primitive search.

## Declarations (in source order)

- L26 `int CountPlayerEntries()`: Returns the player's filtered combat-round queue size (excludes the engine's 0xFF "currently dispatching" placeholder).
- L36 `void ReportPrePressDepth()`: Snapshot the queue depth RIGHT BEFORE the engine dispatches a press.
- L41 `int GetPrePressDepth()`: Returns the depth stashed by the most recent ReportPrePressDepth() call (or -1 if none).
- L50 `void ArmUserQueueAdd()`: Arm the user-attribution latch: "a user press is dispatching a queue add right now." Called from the two user-initiated queue sites — the bare-key dispatch (input_pipeline) and the unified-menu Enter.
- L72 `void ArmNoOpWatch()`: A user press that produces NO queue add — PERSONAL actions only.
- L77 `void CancelNoOpWatch()`: Cancel a pending watch when the caller has already explained the outcome — otherwise KOTOR 1's spoken refusal reason would be followed by the generic line saying the same thing twice.
- L81 `void TickNoOpWatch()`: Per-tick verdict for the watch above.
- L92 `void OnEngineActionAdded(void* combatRound, void* action)`: Announce a queued action the engine just accepted (or rejected at the cap) — the authoritative "X, Platz N" / "Warteschlange voll" cue.
- L96 `bool Open()`: True iff armed (queue had >= 1 entry — empty queue speaks "leer" and returns false).
- L98 `bool IsActive()`
- L101 `bool HandleInputEvent(int code, int value)`: Press-edge only.
- L104 `void ForceDisarm(const char* reason)`: Engine owns the queue — no engine-side cleanup needed.
- L106 `void Tick()`
- L107 `void PollWin32Hotkey()`
