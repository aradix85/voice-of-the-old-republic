# endar_softlock.cpp (337 lines)

Source: `patches/Accessibility/endar_softlock.cpp`

(From endar_softlock.h.) Endar Spire Command Module (END_M01AA) softlock guard + diagnostic net.

The tutorial's room-3 -> room-5 -> bridge sequence is scripted: the west corridor door to the bridge (tag end_door16) only unlocks once the room-5 fight is cleared. If that sequence stalls (e.g. a save/reload mid-cutscene), the door stays locked with no reachable enemies and no in-world explanation, and blind players — guided toward the bridge by the beacon — poke the door repeatedly and give up. This module: 1. Speaks graduated guidance when the player keeps trying end_door16 while room 5 is uncleared (helpful first cue; reload advice after repeats). 2. Edge-logs the plot-state timeline while in the module, so the next stuck report carries the exact sequence that derailed. It never edits engine state, and never stands in for an engine call — narration + logging only.

## Declarations (in source order)

- L106 `bool operator==(const Snapshot& o) const`
- L115 `bool EqualsCI(const char* a, const char* b)`
- L125 `bool IsCommandModule()`
- L131 `Snapshot ReadSnapshot()`
- L145 `void LogSnapshot(const char* why, const Snapshot& s)`
- L153 `void ResetVisit()`
- L166 `int CompanionCount()`: Active companions, PC excluded (the engine's party table counts followers only).
- L175 `void QueueHint(acc::strings::Id which)`: Queue a line to be spoken after the generic "locked" the router is about to emit.
- L183 `void OnDoorLocked(const char* tag)`: The engine just reported `tag` as locked.
- L252 `bool RuleEndarDoorLocked(const char* text)`: Feedback-router rule.
- L271 `void RegisterMsgRule()`
- L275 `bool WouldBlockCompanionAttempt(void* target)`
- L289 `bool BlockCompanionAttempt(void* target)`
- L300 `void OnAreaChanged(void* /*area*/)`
- L313 `void Tick()`
