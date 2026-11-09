# examine_view_effect_names.cpp (1000 lines)

Source: `patches/Accessibility/examine_view_effect_names.cpp`

localized effect-name and effect-icon-name tables.

Split out of examine_view.cpp by the Phase-1 structure pass (refactoring candidate 8). This is pure data: the EFFECT_TYPES enum and the effecticon.2da row ids mapped to display names, one switch table per language, plus the two dispatchers that pick a table from the current language.

These are localized inline rather than through strings.h Get(Id) because they are keyed by raw engine ids, not by mod string ids - a different keyspace from the strings_<lang>.cpp tables. Russian has no table yet and falls back to English.

Moved verbatim; no logic here at all beyond the language switch.

## Declarations (in source order)

- L33 `const char* EffectNameEn(int type)`
- L107 `const char* EffectNameDe(int type)`
- L181 `const char* EffectNameEs(int type)`
- L255 `const char* EffectNameFr(int type)`
- L329 `const char* EffectNameIt(int type)`
- L418 `const char* EffectIconNameEn(int id)`
- L483 `const char* EffectIconNameDe(int id)`
- L548 `const char* EffectIconNameFr(int id)`
- L613 `const char* EffectIconNameIt(int id)`
- L678 `const char* EffectIconNameEs(int id)`
- L742 `const char* EffectNamePl(int type)`
- L816 `const char* EffectIconNamePl(int id)`
- L929 `const char* EffectIconNameK2(int id)`
- L948 `const char* EffectName(int type)`
- L982 `const char* EffectIconName(int iconId)`
