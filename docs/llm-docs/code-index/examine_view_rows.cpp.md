# examine_view_rows.cpp (159 lines)

Source: `patches/Accessibility/examine_view_rows.cpp`

The rows of the examine panel, built from the creature reads.

## Declarations (in source order)

- L22 `void AddRow(int& idx, const char* fmt, ...)`: Appends one formatted row while there is room.
- L31 `void AddText(int& idx, S text)`
- L36 `void AppendEffectRows(void* serverObject, int& idx)`: One row per effect of the object; an unmapped type gets its number.
- L49 `void AppendFeatRows(void* serverCreature, int& idx)`: One row per feat in CSWSCreatureStats.feats (CExoArrayList ).
- L72 `bool EquippedName(void* creature, size_t slotOffset, char* out, size_t cap)`
- L79 `void AddEquipRow(void* creature, int& idx, size_t slotOffset, S fmt)`: An occupied equipment slot gets a row; an empty one none (a sighted player sees what is worn, not a list of empty slots).
- L86 `void AddCreatureStatRows(void* obj, int& idx)`
- L105 `void AddCreatureDetailRows(void* obj, int& idx)`
- L131 `int BuildRows()`
