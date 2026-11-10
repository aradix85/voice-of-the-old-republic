# examine_view_effect_names.cpp (252 lines)

Source: `patches/Accessibility/examine_view_effect_names.cpp`

Spoken names for effects: the EFFECT_TYPES enum and the effecticon.2da rows, keyed by raw engine ids (not strings.h ids). KOTOR 1 names come from the tables here; KOTOR 2 names its icon rows itself through its TLK.

## Declarations (in source order)

- L19 `const char* EffectNameEn(int type)`
- L106 `const char* EffectIconNameEn(int id)`
- L204 `const char* EffectIconNameK2(int id)`
- L220 `const char* EffectName(int type)`
- L244 `const char* EffectIconName(int iconId)`
