# endar_softlock.h (57 lines)

Source: `patches/Accessibility/endar_softlock.h`

Endar Spire Command Module (END_M01AA) softlock guard + diagnostic net.

The tutorial's room-3 -> room-5 -> bridge sequence is scripted: the west corridor door to the bridge (tag end_door16) only unlocks once the room-5 fight is cleared. If that sequence stalls (e.g. a save/reload mid-cutscene), the door stays locked with no reachable enemies and no in-world explanation, and blind players — guided toward the bridge — poke the door repeatedly and give up. This module: 1. Speaks graduated guidance when the player keeps trying end_door16 while room 5 is uncleared (helpful first cue; reload advice after repeats). 2. Edge-logs the plot-state timeline while in the module, so the next stuck report carries the exact sequence that derailed. It never edits engine state, and never stands in for an engine call — narration + logging only.

## Declarations (in source order)

- L22 `void OnAreaChanged(void* area)`: From transitions on area change.
- L26 `void Tick()`: Per-frame, from core_tick.
- L39 `void RegisterMsgRule()`: Register the door-guidance rule with the feedback router.
- L51 `bool BlockCompanionAttempt(void* target)`: Interact-dispatch guard for one case only: the Trask door (end_door19) tried while Trask himself is the party leader (Tab swapped to him).
- L54 `bool WouldBlockCompanionAttempt(void* target)`: The same test without speaking: would BlockCompanionAttempt refuse it?
