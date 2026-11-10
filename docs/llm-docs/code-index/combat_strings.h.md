# combat_strings.h (99 lines)

Source: `patches/Accessibility/combat_strings.h`

Combat-log text: the engine phrases the combat rules parse (anchors) and the shorter labels they speak. The values, with their dialog.tlk strrefs, are in combat_strings.cpp; an anchor that does not match leaves the line on the raw-speech fallback. The field names are the German words the table was first written for.

## Declarations (in source order)

- L11 `struct MsgStrings`
- L96 `const MsgStrings& Get()`
