# engine_reads_items.cpp (815 lines)

Source: `patches/Accessibility/engine_reads_items.cpp`

item / creature domain readers.

Split out of engine_reads.cpp by the Phase-1 structure pass (refactoring candidate 4). engine_reads.cpp kept the generic GUI-control readers (control text, tooltips, TLK lookup, toggle/slider/listbox predicates); this file owns the domain layer that grew on top of them:

* client/server handle translation and item resolution * feat / spell / action description resolvers * item stack, charge and inventory-row reads * the item property-description block reconstruction * creature Force-point reads

Declarations stay in engine_reads.h - splitting that header is a separate, deferred item, so every existing includer is unaffected.

## Declarations (in source order)

- L36 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`
- L53 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`
- L95 `static void* ResolveItemFromServerHandle(uint32_t serverHandle)`
- L111 `static void* GetRulesGlobal()`
- L123 `bool ResolveFeatDescription(uint32_t featIdx, char* outBuf, size_t bufSize)`
- L157 `bool ResolveSpellDescription(uint32_t spellId, char* outBuf, size_t bufSize)`
- L192 `bool ResolveActionDescriptionFromActionId(uint32_t actionId, char* outBuf, size_t bufSize)`
- L221 `int ReadItemStackSize(void* item)`: Read CSWSItem.stack_size (2 bytes) and bit_flags (4 bytes).
- L242 `bool ReadItemChargesRaw(void* item, int* outCharges)`: Read CSWSItem.charges (+0x258) when the item is a charged item (max_charges @ +0x25c > 0).
- L258 `bool IsItemEntryRow(void* control)`
- L274 `uint32_t ReadItemRowHandle(void* rowControl)`: obj_id the row was built for.
- L287 `int ReadItemRowStackCount(void* rowControl)`
- L295 `int ReadItemCharges(void* item)`
- L300 `int ReadItemRowCharges(void* rowControl)`
- L312 `uint32_t GameLanguageId()`: The dialog.tlk LanguageID the game runs in — the `language` half of a CExoLocString substring id.
- L328 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L384 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`
- L416 `int ReadItemStack(void* item)`
- L420 `void* ItemFromActionId(uint32_t actionId)`
- L426 `bool IsInventoryItemRow(void* control)`
- L437 `bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize)`
- L462 `bool ReadItemKeyedPropertyString(void* item, uint8_t key, char* outBuf, size_t bufSize)`
- L492 `bool ReadBaseItemFlags(void* item, uint8_t& itemType, uint8_t& weaponType)`: Read item_type / weapon_type via CSWItem::GetBaseItem (the CSWItem subobject is at offset 0 of CSWSItem, so the item pointer is a valid `this`).
- L524 `void ComputeSectionOffsets(void* item, uint8_t weaponType, size_t& offTags, size_t& offValues, size_t& offProps)`: Append the engine's own per-category builders to ONE accumulator, in the same order and with the same guards GetPropertyDescription uses, recording the visible (strlen) length after each section.
- L591 `bool ReadItemDescriptionViaTlk(void* item, char* outBuf, size_t bufSize, uint32_t& outStrref)`: Resolve the item's identified-description text through the TLK strref, bypassing the (sometimes corrupt) inline CExoLocString substring that GetPropertyDescription/GetString prefer.
- L607 `void CopySlice(char* dst, size_t cap, const char* src, size_t start, size_t end)`: Copy src[start..end) into dst (NUL-terminated, bounded by cap).
- L618 `bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out)`
- L695 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`
- L717 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
- L756 `bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax)`
