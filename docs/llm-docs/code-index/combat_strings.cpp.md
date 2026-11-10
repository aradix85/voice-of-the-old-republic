# combat_strings.cpp (94 lines)

Source: `patches/Accessibility/combat_strings.cpp`

The combat-log text of the English game: the engine phrases the combat rules parse, extracted from dialog.tlk with `kdev combat-strings-extract` (strref map in CombatStringsExtractCommand.cs), and the shorter labels they speak. KOTOR 2's combat block is byte-identical to KOTOR 1's, so one table serves both games.

BuildCompact speaks "<actor> hits critical <target>"; "critically hits" would need a BuildCompact change.

Field order MUST match the declaration order in combat_strings.h (positional init).

## Declarations (in source order)

- L91 `const MsgStrings& Get()`
