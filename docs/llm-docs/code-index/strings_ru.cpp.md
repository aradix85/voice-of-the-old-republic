# strings_ru.cpp (1257 lines)

Source: `patches/Accessibility/strings_ru.cpp`

Russian string table.

Machine-translated draft, same quality bar as strings_fr/it/es.cpp and flagged for a native-speaker pass. Contributors: correcting wording here is safe and self-contained -- only the text inside the quotes should change.

Encoding: Windows-1251 hex escapes for Cyrillic. NOT UTF-8 and NOT Windows-1252. The build does not pass /utf-8, and the bytes are widened with an explicit codepage rather than CP_ACP precisely so that a Russian install still speaks correctly on a German or English Windows. acc::strings::CodepageFor(Lang::Ru) returns 1251 and core_dllmain pins it through prism::SetSpeechCodepage at startup.

Escapes were generated from UTF-8 source, not typed by hand. Where a hex escape is followed by a literal hex digit the string is split with "" so the escape cannot swallow it (the same trick strings_es.cpp uses).

Format placeholders (%s / %d / %u) keep the SAME ORDER as the English table -- callers pass arguments positionally and do not reorder per language. Russian word order differs from English in places; where the natural phrasing would need a different argument order, the wording was adjusted instead of the placeholders.

Combat speech is fed by combat_strings.cpp::kRu (engine anchors extracted from Allard's dialog.tlk); this table covers the Id::* speech path.

## Declarations (in source order)

- L31 `const char* Get(Id id)`
