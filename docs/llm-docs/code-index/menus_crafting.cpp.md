# menus_crafting.cpp (347 lines)

Source: `patches/Accessibility/menus_crafting.cpp`

KOTOR 2 workbench / crafting item-list screens. See menus_crafting.h for the design rationale + decompile evidence.

## Declarations (in source order)

- L70 `void* TitleLabelFor(void* panel, PanelKind k)`: The title label of a K2 workbench-family panel, or nullptr for non-family kinds.
- L92 `void AnnounceFamilyTitleOnFgEdge(void* fg, PanelKind pk, bool edge, bool suppress)`: Speak the family panel's title when it ARRIVES in the foreground.
- L157 `bool IsCraftingKind(PanelKind k)`
- L162 `bool IsWorkbenchFamilyKind(PanelKind k)`
- L167 `bool IsListBoxVisible(void* listBox)`
- L174 `void* FindActiveCraftList(void* panel)`: The item list the user can currently act on: LB_SHOPITEMS in create view, LB_INVITEMS in breakdown view.
- L182 `void* ViewToggleButton(void* panel)`
- L188 `void* CommitButton(void* panel)`
- L196 `bool ToggleViewFromHotkey()`: Q / E — flip create <-> break down.
- L219 `bool IsHiddenCraftingListBox(void* panel, void* listBox)`
- L230 `bool OwnsPanelTitle(void* panel)`
- L235 `void Tick()`
