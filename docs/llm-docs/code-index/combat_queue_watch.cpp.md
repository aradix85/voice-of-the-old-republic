# combat_queue_watch.cpp (187 lines)

Source: `patches/Accessibility/combat_queue_watch.cpp`

The live queue announce: attributing an engine-added action to the user's press, and saying when a press queued nothing.

## Declarations (in source order)

- L44 `bool HostileNearby()`
- L71 `void ArmUserQueueAdd()`
- L73 `void ArmNoOpWatch()`
- L75 `void CancelNoOpWatch()`
- L77 `void TickNoOpWatch()`
- L88 `void OnEngineActionAdded(void* combatRound, void* action)`
- L135 `void TickIdleNotice()`
- L171 `void ReportPrePressDepth()`
- L175 `int GetPrePressDepth()`
