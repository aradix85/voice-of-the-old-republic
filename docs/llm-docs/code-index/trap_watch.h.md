# trap_watch.h (53 lines)

Source: `patches/Accessibility/trap_watch.h`

Trap ("mine") detection watcher — sighted-parity trap awareness. Engine model: docs/llm-docs/mine-trap-model.md.

The engine tracks per-trap "detected-by" id lists (Awareness checks in CSWSCreature::UpdateMineCheck) and shows sighted players a red ground overlay for every detected hostile mine. This module mirrors exactly that state — a trap counts as visible when its detected-by list holds a party member — so blind players learn about a trap at the same moment a sighted player sees it appear, never earlier.

Three outputs: 1. Ground mines (trigger kind): the engine already emits a feedback line ("<name> entdeckt <Minenname>: ...") through the combat message funnel. We only enrich it — combat.cpp's RuleMineDetect correlates the line with the freshest new detection via PeekFreshMine/ConsumeFreshMine and appends clock direction + distance. 2. Trapped doors/placeables: the engine broadcasts NO feedback line for those (UpdateMineCheck only messages in the trigger branch). We announce "Falle entdeckt: <name>, auf X Uhr, Y Meter" on the detected transition ourselves. 3. Proximity warning: a detected ground mine within a small radius speaks its name + clock + metres ONCE per approach (re-arms only after walking away) — deliberately not repeating, per user decision: you either disarm it or step over it on purpose.

## Declarations (in source order)

- L38 `void Tick()`: Per-frame tick (internally throttled).
- L44 `void ScanNow()`: Rate-limited forced rescan (min 100ms spacing).
- L49 `bool PeekFreshMine(char* nameOut, size_t nameSize, Vector& posOut)`: Freshest not-yet-consumed ground-mine detection (recorded when a trigger trap's detected-by list first gains a party member; entries expire after a few seconds).
- L50 `void ConsumeFreshMine()`
