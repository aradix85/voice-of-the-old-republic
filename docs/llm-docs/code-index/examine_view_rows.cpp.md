# examine_view_rows.cpp (262 lines)

Source: `patches/Accessibility/examine_view_rows.cpp`

The rows of the examine panel, built from the creature reads.

## Declarations (in source order)

- L16 `int AppendEffectRows(void* serverObject, char rows[][192], int& outIdx, int rowCap)`: Walk CSWSObject.effects (CExoArrayList @+0x124) and emit rows for up to `cap` effects.
- L55 `int AppendFeatRows(void* serverCreature, char rows[][192], int& outIdx, int rowCap)`: Walk CSWSCreatureStats.feats (CExoArrayList @+0x0) and emit rows for up to `cap` feats with localized names from CSWRules::GetFeat.
- L97 `int BuildRows()`: (Re)build the row list for the cached target.
