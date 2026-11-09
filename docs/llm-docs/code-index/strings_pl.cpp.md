# strings_pl.cpp (1218 lines)

Source: `patches/Accessibility/strings_pl.cpp`

Polish string table.

Machine-translated draft, same quality bar as strings_fr/it/es/ru.cpp and flagged for a native-speaker pass. Contributors: correcting wording here is safe and self-contained -- only the text inside the quotes should change.

Encoding: Windows-1250 hex escapes for the Polish letters. NOT UTF-8 and NOT Windows-1252 -- a and c and e and l and n and s and z with their diacritics do not exist in 1252 at all, so this table cannot be written the way the German or French ones are. acc::strings::CodepageFor(Lang::Pl) returns 1250 and core_dllmain pins it through prism::SetSpeechCodepage before the first utterance, which is what makes a Polish install speak correctly on an English or German Windows.

Escapes were generated from UTF-8 source by tools/scratch escape-cp1250.ps1, not typed by hand. Where a hex escape is followed by a literal hex digit the string is split with "" so the escape cannot swallow it -- the same trick strings_es.cpp and strings_ru.cpp use.

Format placeholders (%s / %d / %u) keep the SAME ORDER as the English table -- callers pass arguments positionally and do not reorder per language. Where Polish word order wanted a different argument order, the wording was adjusted instead of the placeholders.

Combat speech is fed by combat_strings.cpp::kPl (engine anchors extracted from the LEM dialog.tlk); this table covers the Id::* speech path.

## Declarations (in source order)

- L32 `const char* Get(Id id)`
