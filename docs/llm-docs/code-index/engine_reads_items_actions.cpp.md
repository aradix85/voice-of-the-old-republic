# engine_reads_items_actions.cpp (106 lines)

Source: `patches/Accessibility/engine_reads_items_actions.cpp`

The rules rows of feats and Force powers, and the descriptions behind an action-bar action id: feats, Force powers and items.

## Declarations (in source order)

- L23 `void* GetRulesGlobal()`
- L31 `void* GetFeatRow(uint16_t featIdx)`
- L41 `void* GetSpellRow(int spellId)`
- L57 `bool ReadRowStrRefText(void* row, size_t offset, char* outBuf, size_t bufSize)`: Through the dual-game TLK path: the engine's own Get*Text accessors were KOTOR 1-only addresses.
- L66 `bool ResolveFeatDescription(uint32_t featIdx, char* outBuf, size_t bufSize)`
- L71 `bool ResolveSpellDescription(uint32_t spellId, char* outBuf, size_t bufSize)`
- L78 `bool ResolveActionDescriptionFromActionId(uint32_t actionId, char* outBuf, size_t bufSize)`
- L99 `void* ItemFromActionId(uint32_t actionId)`
