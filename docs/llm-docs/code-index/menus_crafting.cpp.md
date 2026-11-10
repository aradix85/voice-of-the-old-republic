# menus_crafting.cpp (246 lines)

Source: `patches/Accessibility/menus_crafting.cpp`

The K2 workbench screens as a whole (menus_crafting.h has the design): the per-tick watcher with the auto-open redirect, the title announce on a foreground edge, the Q/E view toggle, and the chain filters.

## Declarations (in source order)

- L46 `void* TitleLabelFor(void* panel, PanelKind k)`
- L65 `void AnnounceFamilyTitleOnFgEdge(void* fg, PanelKind pk, bool edge, bool suppress)`: The family panel's title when it arrives in the foreground (see OwnsPanelTitle).
- L99 `bool RedirectAutoOpenedUpgrade(void* fg, PanelKind pk, bool edge)`: The K2 workbench dialog reply pushes CSWGuiUpgrade (the first upgradeable item's slots) on top of the item list, so keyboard users land inside "Upgrade: " instead of the list.
- L121 `void WatchList(void* fg, PanelKind pk)`: A create/breakdown flip is spoken and both it and a changed row count (category filter, an item made or broken down) rebind the chain.
- L162 `bool IsCraftingKind(PanelKind k)`
- L166 `bool IsWorkbenchFamilyKind(PanelKind k)`
- L171 `bool IsListBoxVisible(void* listBox)`
- L177 `void* FindActiveCraftList(void* panel)`: LB_SHOPITEMS in the create view, LB_INVITEMS in the breakdown view; null when neither is visible (still building) or off the crafting screens.
- L187 `void* ViewToggleButton(void* panel)`: Null off the two crafting screens (and on K1, where the member offsets poison).
- L191 `void* CommitButton(void* panel)`
- L197 `bool ToggleViewFromHotkey()`: Like store::ToggleModeFromHotkey: the engine's own button, fired from a key; Tick's view-flip edge speaks the result.
- L212 `bool IsHiddenCraftingListBox(void* panel, void* listBox)`
- L221 `bool OwnsPanelTitle(void* panel)`
- L225 `void Tick()`
