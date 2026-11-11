# menus_inventory.cpp (109 lines)

Source: `patches/Accessibility/menus_inventory.cpp`

(From menus_inventory.h.) inventory panel (CSWGuiInGameInventory) helpers.

Sibling of menus_journal.h, and for the same reason: the "Inventory" screen has buttons that mutate the item list out from under our cached chain, and one button that duplicates what Enter on a row already does.

The engine model behind these three predicates is written up next to kInventoryItemListBoxOffset in engine_offsets_fields.h — read that first. In short:

* "Use" (useitem_button) is the gamepad-A shortcut. It dispatches activate to panel.active_control, which our cursor warp routinely fails to update, so it can fire the wrong item's handler. Enter on the row reaches the same per-item handler directly. * "Show: …" (the filter button) repopulates the list lazily in Draw(), leaving the chain with a stale row count and row→item mapping.

## Declarations (in source order)

- L24 `bool IsButtonAtOffset(void* panel, void* control, std::size_t offset)`: Embedded button objects live AT panel+offset (the ctor constructs them in place via &this->useitem_button etc.), so the chain captures &button == that address.
- L32 `bool IsUseItemButton(void* panel, void* control)`
- L36 `bool IsFilterButton(void* panel, void* control)`
- L53 `void SyncListBoxSelection(void* panel, void* row)`
- L88 `void ForceRepopulate(void* panel)`
