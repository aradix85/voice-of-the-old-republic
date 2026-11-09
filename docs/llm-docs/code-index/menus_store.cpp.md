# menus_store.cpp (632 lines)

Source: `patches/Accessibility/menus_store.cpp`

store / trading panel (CSWGuiStore). See menus_store.h for the design rationale.

## Declarations (in source order)

- L32 `uint32_t ReadListBoxControlBitFlags(void* panel, size_t listOffset)`: Read CSWGuiControl.bit_flags @ +0x44 on the listbox at panel + listOffset.
- L47 `Mode ResolveMode(void* panel)`: Resolve the current Buy/Sell mode from the visibility bit on the two item listboxes.
- L60 `uint32_t ReadRowObjId(void* row)`: Read the row's obj_id at +0x1c4.
- L74 `bool IsStoreItemEntry(void* control)`: True iff `control` is a CSWGuiStoreItemEntry (rows of either listbox) rather than one of the action buttons (cancel / examine / accept) the chain also surfaces.
- L88 `int ReadListBoxSize(void* panel, size_t listOffset)`: Read the listbox controls.size at panel + listOffset.
- L103 `inline void* ResolveItemFromHandle(uint32_t clientHandle)`: Thin alias to the shared resolver in engine_reads.h.
- L111 `uint32_t CallGetItemValue(uintptr_t fnAddr, void* storePanel, void* item)`
- L127 `int ReadItemStock(void* item, bool& outFinite)`: Read CSWSItem.stack_size + check the infinite-stock bit.
- L182 `uint32_t ReadStorePlayerGold(void* panel)`
- L194 `bool IsStorePanel(void* panel)`
- L205 `bool IsHiddenStoreListBox(void* panel, void* listBox)`
- L220 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L283 `void TickMonitorMode()`
- L454 `bool IsStoreItemRow(void* control)`
- L460 `void DispatchTradeAction(void* panel, void* row)`
- L583 `bool ToggleModeFromHotkey()`
- L607 `bool CloseFromEsc()`
