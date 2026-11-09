# menus_inventory.h (57 lines)

Source: `patches/Accessibility/menus_inventory.h`

inventory panel (CSWGuiInGameInventory) helpers.

Sibling of menus_journal.h, and for the same reason: the "Inventar" screen has buttons that mutate the item list out from under our cached chain, and one button that duplicates what Enter on a row already does.

The engine model behind these three predicates is written up next to kInventoryItemListBoxOffset in engine_offsets_fields.h — read that first. In short:

* "Verwenden" (useitem_button) is the gamepad-A shortcut. It dispatches activate to panel.active_control, which our cursor warp routinely fails to update, so it can fire the wrong item's handler. Enter on the row reaches the same per-item handler directly. * "Zeigen: …" (the filter button) repopulates the list lazily in Draw(), leaving the chain with a stale row count and row→item mapping.

## Declarations (in source order)

- L24 `bool IsUseItemButton(void* panel, void* control)`: True iff `control` is the panel's "Verwenden" button (panel+0x1328).
- L29 `bool IsFilterButton(void* panel, void* control)`: True iff `control` is the item-filter cycle button (panel+0x14ec, the one captioned "Zeigen: ").
- L47 `void SyncListBoxSelection(void* panel, void* row)`: Point item_listbox.selection_index at `row` before its activate is dispatched.
- L54 `void ForceRepopulate(void* panel)`: Force a synchronous rebuild of the item list by calling the engine's CSWGuiInGameInventory::PopulateItemListBox.
