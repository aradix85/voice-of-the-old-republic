# engine_levelup.h (54 lines)

Source: `patches/Accessibility/engine_levelup.h`

Level-up screen trigger.

Calls CGuiInGame::ShowLevelUpGUI @0x0062dc00 — same surface the character sheet btn_levelup click reaches via CSWGuiInGameCharacter::ShowLevelUpGUI @0x006b0bb0. Going through the CGuiInGame variant works without the InGameCharacter panel constructed; the engine surfaces the level-up panel from the CGuiInGame singleton.

param_1 unknown — Ghidra has no decompile (.text packed). Passing 0 is the current guess; iterate if wrong.

## Declarations (in source order)

- L24 `bool PlayerCanLevelUp()`: True iff the active leader has actually earned the next level, per the engine's own CSWSCreatureStats::CanLevelUp @0x005a6810 — the same predicate that drives the character sheet btn_levelup enabled state (checks leve...
- L28 `bool CreatureCanLevelUp(void* serverCreature)`: Same predicate for any server creature (party members).
- L35 `bool TriggerLevelUp()`: True iff chain resolved + dispatched without SEH.
- L45 `void TickLevelUpPause()`: Per-frame maintenance for the level-up overlay pause.
- L51 `bool IsOpeningLevelUp()`: True only during the synchronous ShowLevelUpGUI dispatch, when the wizard's SkillInfoBox sub-panel first-sight fires before the wizard is on the modal stack (so HasActiveLevelUpPanel() is not yet true).
