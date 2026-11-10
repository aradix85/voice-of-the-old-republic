# combat_queue_watch.cpp (116 lines)

Source: `patches/Accessibility/combat_queue_watch.cpp`

The live queue announce: attributing an engine-added action to the user's press, and saying when a press queued nothing.

## Declarations (in source order)

- L36 `void ArmUserQueueAdd()`
- L38 `void ArmNoOpWatch()`
- L40 `void CancelNoOpWatch()`
- L42 `void TickNoOpWatch()`
- L53 `void OnEngineActionAdded(void* combatRound, void* action)`
- L100 `void ReportPrePressDepth()`
- L104 `int GetPrePressDepth()`
