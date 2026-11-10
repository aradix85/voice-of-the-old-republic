# combat_queue_watch.cpp (127 lines)

Source: `patches/Accessibility/combat_queue_watch.cpp`

The live queue announce: attributing an engine-added action to the user's press, and saying when a press queued nothing.

## Declarations (in source order)

- L37 `void ArmUserQueueAdd()`
- L39 `void ArmNoOpWatch()`
- L41 `void CancelNoOpWatch()`
- L43 `void TickNoOpWatch()`
- L54 `void OnEngineActionAdded(void* combatRound, void* action)`
- L111 `void ReportPrePressDepth()`
- L115 `int GetPrePressDepth()`
