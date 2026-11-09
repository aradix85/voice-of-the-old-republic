# combat_strings.cpp (938 lines)

Source: `patches/Accessibility/combat_strings.cpp`

Combat-message localization table — DE + EN tables.

Engine anchors were extracted from each locale's dialog.tlk via `kdev combat-strings-extract` (the strref→field map lives in CombatStringsExtractCommand.cs); speech-side labels are hand-translated.

Encoding: hex escapes in the *game's* codepage, because the anchors are memcmp'd/strstr'd directly against CExoString bytes. For de/en/fr/it/es that is Windows-1252 (\xFC=ü, \xF6=ö, \xE4=ä, \xDF=ß). kRu and kPl are the exceptions: Russian installs are Windows-1251 and Polish ones Windows-1250, so those escapes must not be read as 1252. acc::strings::CodepageFor(Lang) reports which, and acc::speech::SetSpeechCodepage pins it for the speech path.

Field order in the initialiser MUST match the declaration order in combat_strings.h — we use positional init (no designated initialisers in this codebase, per patch-build script flag set).

## Declarations (in source order)

- L760 `MsgStrings BuildDeK2()`: ---- KOTOR 2 anchor deltas ------------------------------------------------ The K2 German dialog.tlk keeps every combat strref the parser depends on (42042-42150 summary/breakdown block, 1374-1456 save/damage/absorb b...
- L788 `MsgStrings BuildRuK2()`: The K2 Russian TSLRCM TLK (Workshop item 2143250983, cached 2026-08-12 as dialog_ru_k2.tlk) is a from-scratch re-translation relative to K1's Allard TLK — unlike German's four-anchor drift, nearly every engine anchor ...
- L828 `MsgStrings BuildFrK2()`: French: the 2004 K2 localization kept nearly all of K1's combat phrasing — only four anchors drift.
- L839 `MsgStrings BuildItK2()`: Italian: re-translated breakdown vocabulary ("Analisi di ..." replaces K1's "Divisione ..."), new hit/miss verbs, new auto-tags.
- L868 `MsgStrings BuildEsK2()`: Spanish: re-translated breakdown vocabulary ("Desglose de ..." replaces K1's "Colapso ..."), new hit/miss phrasing; the save line moved its inverted exclamation mark into the type/result separator.
- L896 `const MsgStrings& Get()`
