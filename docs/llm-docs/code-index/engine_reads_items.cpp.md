# engine_reads_items.cpp (236 lines)

Source: `patches/Accessibility/engine_reads_items.cpp`

Item and creature reads on top of the generic readers of engine_reads.cpp: client/server handle translation and item resolution, stack, charge and inventory-row reads, item names in another language, the workbench slots and a creature's Force points. Item descriptions are in engine_reads_items_desc.cpp, action descriptions in _actions.cpp; all are declared in engine_reads.h.

## Declarations (in source order)

- L25 `bool IsItemEntryRow(void* control)`
- L32 `uint32_t ReadItemRowHandle(void* rowControl)`: The obj_id the row was built for (same offset on both row classes); 0 when the control is no row or the read faults.
- L41 `bool ReadItemChargesRaw(void* item, int* outCharges)`: Charged items (max_charges > 0) cannot stack; false for any other item.
- L54 `void* ResolveItemFromRow(void* rowControl)`
- L62 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`
- L79 `void* ResolveItemFromServerHandle(uint32_t serverHandle)`
- L92 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`
- L98 `int ReadItemStack(void* item)`: CSWSItem.stack_size; 0 on a fault and for infinite-stock items (the store reads that flag itself).
- L108 `int ReadItemRowStackCount(void* rowControl)`
- L112 `int ReadItemCharges(void* item)`
- L117 `int ReadItemRowCharges(void* rowControl)`
- L122 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L168 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`
- L193 `bool IsInventoryItemRow(void* control)`
- L198 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`
- L209 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
