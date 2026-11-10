# engine_reads_items_actions.cpp (143 lines)

Source: `patches/Accessibility/engine_reads_items_actions.cpp`

Descriptions behind an action-bar action id: feats, Force powers and items.

## Declarations (in source order)

- L36 `static void* GetRulesGlobal()`
- L46 `bool ResolveFeatDescription(uint32_t featIdx, char* outBuf, size_t bufSize)`
- L78 `bool ResolveSpellDescription(uint32_t spellId, char* outBuf, size_t bufSize)`
- L113 `bool ResolveActionDescriptionFromActionId(uint32_t actionId, char* outBuf, size_t bufSize)`
- L136 `void* ItemFromActionId(uint32_t actionId)`
