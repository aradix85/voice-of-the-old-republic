# menus_crafting_internal.h (20 lines)

Source: `patches/Accessibility/menus_crafting_internal.h`

Shared between the menus_crafting files: the workbench panel kinds, the visible craft list, and the latch that tells Tick the user opened the upgrade screen from the list.

## Declarations (in source order)

- L12 `bool IsCraftingKind(acc::engine::PanelKind k)`
- L13 `bool IsWorkbenchFamilyKind(acc::engine::PanelKind k)`
- L14 `bool IsListBoxVisible(void* listBox)`
- L15 `void* FindActiveCraftList(void* panel)`

Also 1 constants or variables at namespace scope.
