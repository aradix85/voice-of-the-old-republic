# combat_queue_watch.cpp (185 lines)

Source: `patches/Accessibility/combat_queue_watch.cpp`

The live queue announce: attributing an engine-added action to the user's press, and saying when a press queued nothing.

## Declarations (in source order)

- L43 `bool HostileNearby()`
- L70 `void ArmUserQueueAdd()`
- L72 `void ArmNoOpWatch()`
- L74 `void CancelNoOpWatch()`
- L76 `void TickNoOpWatch()`
- L87 `void OnEngineActionAdded(void* combatRound, void* action)`
- L134 `void TickIdleNotice()`
- L169 `void ReportPrePressDepth()`
- L173 `int GetPrePressDepth()`
