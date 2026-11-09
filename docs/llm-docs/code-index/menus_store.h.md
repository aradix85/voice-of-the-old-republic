# menus_store.h (101 lines)

Source: `patches/Accessibility/menus_store.h`

store / trading panel (CSWGuiStore).

Two-mode trading screen. Same panel kind for both modes; the engine flips one bit on the shop / inv listbox's CSWGuiControl.bit_flags and swaps which listbox is visible. The price displayed in the static cost_value label is whichever value function fits the active mode: * Buy mode  (shopitems_listbox.bit_flags & 2 != 0) → GetItemBuyValue  (what the merchant charges). * Sell mode (invitems_listbox.bit_flags & 2 != 0) → GetItemSellValue (what the merchant pays).

Both checks are language-agnostic. We don't read the title label or button text. See engine_offsets.h kStore* for the field map.

What we add for screen readers: * Chain-step suffix per item row — price + stock count. * Mode-change announcement when the user flips Buy↔Sell.

We don't reshape the chain itself even though it carries rows from both lists (the engine packs both listboxes' rows into panel.controls regardless of which is visible). Filtering chain rows by mode is a separate follow-up.

## Declarations (in source order)

- L29 `bool IsStorePanel(void* panel)`: True iff `panel` is a CSWGuiStore (vtable equality).
- L41 `bool IsHiddenStoreListBox(void* panel, void* listBox)`: True iff `panel` is a Store AND `listBox` is one of its two item listboxes (shopitems or invitems) and that listbox is currently HIDDEN — i.e.
- L50 `void AnnounceChainStepSuffix(void* panel, void* control)`: Speak the per-row price + stock suffix from the chain-step input handler, right after AnnounceControl spoke the item name.
- L63 `void TickMonitorMode()`: Per-tick hook.
- L70 `bool IsStoreItemRow(void* control)`: True iff `control` is a CSWGuiStoreItemEntry — a row of either the shop or inv listbox.
- L86 `void DispatchTradeAction(void* panel, void* row)`: Drain handler for the Kind::StoreItemActivate pending op.
- L93 `bool ToggleModeFromHotkey()`: Synthesize a click on the store's examine_button (mode toggle).
- L98 `bool CloseFromEsc()`: Synthesize a click on the store's cancel_button (Schliess.).
