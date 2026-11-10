# combat_queue_watch.cpp (189 lines)

Source: `patches/Accessibility/combat_queue_watch.cpp`

The live queue announce: attributing an engine-added action to the user's press, and saying when a press queued nothing.

## Declarations (in source order)

- L44 `bool HostileNearby()`
- L72 `void ArmUserQueueAdd()`
- L74 `void ArmNoOpWatch()`
- L76 `void CancelNoOpWatch()`
- L78 `void TickNoOpWatch()`
- L89 `void OnEngineActionAdded(void* combatRound, void* action)`
- L136 `void TickIdleNotice()`
- L173 `void ReportPrePressDepth()`
- L177 `int GetPrePressDepth()`
