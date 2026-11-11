# menus_store.cpp (371 lines)

Source: `patches/Accessibility/menus_store.cpp`

The store (CSWGuiStore; menus_store.h has the design): buy/sell mode from the visible item list, the price and stock suffix on a chain step, the trade dispatch with its outcome watcher, and the G and Esc keys.

## Declarations (in source order)

- L36 `const char* ModeName(Mode m)`
- L38 `size_t ListOffset(Mode m)`
- L44 `bool ListVisible(void* panel, size_t listOffset)`: The engine sets the visible bit on whichever item list shows (ShowBuyGUI / ShowSellGUI); a failed read counts as hidden.
- L52 `Mode ResolveMode(void* panel)`: Unknown only in the moment before InitializeStoreType runs.
- L59 `int ListSize(void* panel, Mode m)`
- L64 `void* RowItem(void* row)`: The item behind a store row (CSWGuiStoreItemEntry).
- L69 `uint32_t ItemPrice(void* panel, Mode m, void* item)`: GetItemBuyValue / GetItemSellValue on the live panel; 0 on a fault.
- L83 `int ReadItemStock(void* item, bool& outFinite)`: CSWSItem stack_size, a ushort (the engine reads it as an int; the upper half is padding).
- L98 `void* ForegroundStore()`
- L131 `void ResetTracking()`
- L141 `void OnModeChange(void* fg, Mode current)`: The chain is panel-keyed and the panel stays, so without a rebind the user would walk the hidden list.
- L153 `void ResolveTradeWatch(void* fg, Mode current, bool sizeChanged, int currentSize)`
- L189 `bool IsStorePanel(void* panel)`
- L193 `bool IsStoreItemRow(void* control)`
- L197 `bool IsHiddenStoreListBox(void* panel, void* listBox)`
- L207 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L244 `void TickMonitorMode()`
- L280 `void DispatchTradeAction(void* panel, void* row)`
- L342 `bool ToggleModeFromHotkey()`
- L356 `bool CloseFromEsc()`
