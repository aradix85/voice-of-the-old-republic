# combat_strings.cpp (93 lines)

Source: `patches/Accessibility/combat_strings.cpp`

The combat-log text of the English game: the engine phrases the combat rules parse (from dialog.tlk, each with its strref) and the shorter labels they speak. KOTOR 2's combat block is byte-identical to KOTOR 1's, so one table serves both games.

BuildCompact speaks "<actor> hits critical <target>"; "critically hits" would need a BuildCompact change.

Field order MUST match the declaration order in combat_strings.h (positional init).

## Declarations (in source order)

- L90 `const MsgStrings& Get()`
