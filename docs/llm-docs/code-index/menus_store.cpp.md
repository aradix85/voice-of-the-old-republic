# menus_store.cpp (376 lines)

Source: `patches/Accessibility/menus_store.cpp`

The store (CSWGuiStore; menus_store.h has the design): buy/sell mode from the visible item list, the price and stock suffix on a chain step, the trade dispatch with its outcome watcher, and the G and Esc keys.

## Declarations (in source order)

- L35 `const char* ModeName(Mode m)`
- L37 `size_t ListOffset(Mode m)`
- L43 `bool ListVisible(void* panel, size_t listOffset)`: The engine sets the visible bit on whichever item list shows (ShowBuyGUI / ShowSellGUI); a failed read counts as hidden.
- L51 `Mode ResolveMode(void* panel)`: Unknown only in the moment before InitializeStoreType runs.
- L58 `int ListSize(void* panel, Mode m)`
- L62 `uint32_t ReadU32Or0(void* base, size_t offset)`
- L69 `void* RowItem(void* row)`: The item behind a store row (CSWGuiStoreItemEntry).
- L74 `uint32_t ItemPrice(void* panel, Mode m, void* item)`: GetItemBuyValue / GetItemSellValue on the live panel; 0 on a fault.
- L88 `int ReadItemStock(void* item, bool& outFinite)`: CSWSItem stack_size, a ushort (the engine reads it as an int; the upper half is padding).
- L103 `void* ForegroundStore()`
- L136 `void ResetTracking()`
- L146 `void OnModeChange(void* fg, Mode current)`: The chain is panel-keyed and the panel stays, so without a rebind the user would walk the hidden list.
- L158 `void ResolveTradeWatch(void* fg, Mode current, bool sizeChanged, int currentSize)`
- L194 `bool IsStorePanel(void* panel)`
- L198 `bool IsStoreItemRow(void* control)`
- L202 `bool IsHiddenStoreListBox(void* panel, void* listBox)`
- L212 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L249 `void TickMonitorMode()`
- L285 `void DispatchTradeAction(void* panel, void* row)`
- L347 `bool ToggleModeFromHotkey()`
- L361 `bool CloseFromEsc()`
