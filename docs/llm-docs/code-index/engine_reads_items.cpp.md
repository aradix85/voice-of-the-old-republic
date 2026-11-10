# engine_reads_items.cpp (234 lines)

Source: `patches/Accessibility/engine_reads_items.cpp`

Item and creature reads on top of the generic readers of engine_reads.cpp: client/server handle translation and item resolution, stack, charge and inventory-row reads, item names in another language, the workbench slots and a creature's Force points. Item descriptions are in engine_reads_items_desc.cpp, action descriptions in _actions.cpp; all are declared in engine_reads.h.

## Declarations (in source order)

- L25 `uintptr_t ReadVtable(void* control)`
- L31 `bool IsItemEntryRow(void* control)`
- L38 `uint32_t ReadItemRowHandle(void* rowControl)`: The obj_id the row was built for (same offset on both row classes); 0 when the control is no row or the read faults.
- L47 `bool ReadItemChargesRaw(void* item, int* outCharges)`: Charged items (max_charges > 0) cannot stack; false for any other item.
- L60 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`
- L77 `void* ResolveItemFromServerHandle(uint32_t serverHandle)`
- L90 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`
- L96 `int ReadItemStack(void* item)`: CSWSItem.stack_size; 0 on a fault and for infinite-stock items (the store reads that flag itself).
- L106 `int ReadItemRowStackCount(void* rowControl)`
- L110 `int ReadItemCharges(void* item)`
- L115 `int ReadItemRowCharges(void* rowControl)`
- L120 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L166 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`
- L191 `bool IsInventoryItemRow(void* control)`
- L196 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`
- L207 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
