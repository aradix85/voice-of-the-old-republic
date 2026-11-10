# engine_reads_items.cpp (392 lines)

Source: `patches/Accessibility/engine_reads_items.cpp`

item / creature domain readers.

Split out of engine_reads.cpp by the Phase-1 structure pass (refactoring candidate 4). engine_reads.cpp kept the generic GUI-control readers (control text, tooltips, TLK lookup, toggle/slider/listbox predicates); this file owns the domain layer that grew on top of them:

* client/server handle translation and item resolution * feat / spell / action description resolvers * item stack, charge and inventory-row reads * the item property-description block reconstruction * creature Force-point reads

Declarations stay in engine_reads.h - splitting that header is a separate, deferred item, so every existing includer is unaffected.

## Declarations (in source order)

- L40 `int ReadItemStackSize(void* item)`: Read CSWSItem.stack_size (2 bytes) and bit_flags (4 bytes).
- L61 `bool ReadItemChargesRaw(void* item, int* outCharges)`: Read CSWSItem.charges (+0x258) when the item is a charged item (max_charges @ +0x25c > 0).
- L77 `bool IsItemEntryRow(void* control)`
- L93 `uint32_t ReadItemRowHandle(void* rowControl)`: obj_id the row was built for.
- L106 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`
- L123 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`
- L141 `void* ResolveItemFromServerHandle(uint32_t serverHandle)`
- L157 `int ReadItemRowStackCount(void* rowControl)`
- L165 `int ReadItemCharges(void* item)`
- L170 `int ReadItemRowCharges(void* rowControl)`
- L178 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L225 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`
- L257 `int ReadItemStack(void* item)`
- L261 `bool IsInventoryItemRow(void* control)`
- L272 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`
- L294 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
- L333 `bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax)`
