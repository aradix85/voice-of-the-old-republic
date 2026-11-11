# menus_monitors_inventory.cpp (80 lines)

Source: `patches/Accessibility/menus_monitors_inventory.cpp`

Repairing the inventory's lazily filled item list: when the list is still empty with the screen in front, run the engine's repopulate once.

## Declarations (in source order)

- L41 `int InventoryItemRowCount(void* panel)`: Rows currently in the inventory's item listbox.
- L48 `void MonitorInventoryListPopulation()`
