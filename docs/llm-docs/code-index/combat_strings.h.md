# combat_strings.h (167 lines)

Source: `patches/Accessibility/combat_strings.h`

Combat-message localization table.

Holds locale-dependent tokens the combat msg-bus rules use: engine- side parse anchors (substrings the engine emits that we scan for) and our shortened output labels (what we speak). Kept separate from strings.h so the speech-only Id enum stays focused on what the user hears, not what the engine says.

Language follows acc::strings::Lang.

All five locales (DE/EN/FR/IT/ES) carry a full anchor set. DE is engine- verified against a real combat log; EN/FR/IT/ES anchors were extracted mechanically from each locale's dialog.tlk (see CombatStringsExtractCommand.cs StrrefMap) and the same rules reproduce the DE reference byte-for-byte, but the non-DE sets still want one in- locale combat capture to confirm (grep `MsgBuf: raw:` vs `emit-*`). Any anchor that doesn't match falls through to raw speech — no regression.

## Declarations (in source order)

- L23 `struct MsgStrings`
- L164 `const MsgStrings& Get()`
