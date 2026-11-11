# engine_reads_items.cpp (228 lines)

Source: `patches/Accessibility/engine_reads_items.cpp`

Item and creature reads on top of the generic readers of engine_reads.cpp: client/server handle translation and item resolution, stack, charge and inventory-row reads, item names in another language, the workbench slots and a creature's Force points. Item descriptions are in engine_reads_items_desc.cpp, action descriptions in _actions.cpp; all are declared in engine_reads.h.

## Declarations (in source order)

- L25 `bool IsItemEntryRow(void* control)`
- L32 `uint32_t ReadItemRowHandle(void* rowControl)`: The obj_id the row was built for (same offset on both row classes); 0 when the control is no row or the read faults.
- L41 `bool ReadItemChargesRaw(void* item, int* outCharges)`: Charged items (max_charges > 0) cannot stack; false for any other item.
- L54 `uint32_t ClientToServerObjectId(uint32_t clientHandle)`
- L71 `void* ResolveItemFromServerHandle(uint32_t serverHandle)`
- L84 `void* ResolveItemFromClientHandle(uint32_t clientHandle)`
- L90 `int ReadItemStack(void* item)`: CSWSItem.stack_size; 0 on a fault and for infinite-stock items (the store reads that flag itself).
- L100 `int ReadItemRowStackCount(void* rowControl)`
- L104 `int ReadItemCharges(void* item)`
- L109 `int ReadItemRowCharges(void* rowControl)`
- L114 `bool ReadLocStringOtherLanguage(void* base, size_t locStringOffset, char* outBuf, size_t bufSize)`
- L160 `bool ReadItemRowName(void* rowControl, char* outBuf, size_t bufSize)`
- L185 `bool IsInventoryItemRow(void* control)`
- L190 `void* GetWorkbenchSlotInstalledItem(void* upgradePanel, void* slotControl)`
- L201 `WorkbenchPickerInfo GetWorkbenchPickerInfo(void* upgradePanel)`
