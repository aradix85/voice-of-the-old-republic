# combat_strings.cpp (94 lines)

Source: `patches/Accessibility/combat_strings.cpp`

The combat-log text of the English game: the engine phrases the combat rules parse (from dialog.tlk, each with its strref) and the shorter labels they speak. KOTOR 2's combat block is byte-identical to KOTOR 1's, so one table serves both games.

word_critical follows the verb in the log line of an unspoken attack ("<actor> hits critical <target>", BuildCompact) and a comma in the spoken lines (", critical").

Field order MUST match the declaration order in combat_strings.h (positional init).

## Declarations (in source order)

- L91 `const MsgStrings& Get()`
