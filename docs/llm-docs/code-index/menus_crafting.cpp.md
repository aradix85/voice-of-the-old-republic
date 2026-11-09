# menus_crafting.cpp (635 lines)

Source: `patches/Accessibility/menus_crafting.cpp`

KOTOR 2 workbench / crafting item-list screens. See menus_crafting.h for the design rationale + decompile evidence.

## Declarations (in source order)

- L65 `bool IsCraftingKind(PanelKind k)`
- L70 `bool IsWorkbenchFamilyKind(PanelKind k)`
- L98 `void* TitleLabelFor(void* panel, PanelKind k)`: The title label of a K2 workbench-family panel, or nullptr for non-family kinds.
- L120 `void AnnounceFamilyTitleOnFgEdge(void* fg, PanelKind pk, bool edge, bool suppress)`: Speak the family panel's title when it ARRIVES in the foreground.
- L168 `bool IsListBoxVisible(void* listBox)`
- L175 `void* FindActiveCraftList(void* panel)`: The item list the user can currently act on: LB_SHOPITEMS in create view, LB_INVITEMS in breakdown view.
- L185 `int RowIndexIn(void* listBox, void* row)`: Index of `row` in the listbox's controls array, or -1.
- L201 `bool WriteListBoxSelection(void* listBox, int index)`
- L235 `bool SelectRow(void* listBox, void* row, int index)`: Make `row` the engine's selected row of `listBox`: what a mouse click on the row leaves behind, minus the click.
- L247 `void* ResolveRowItem(void* row)`: The CSWSItem* behind a crafting row, or null.
- L263 `bool IsRowCraftable(void* row)`: Can the character actually make (or break down) this row?
- L278 `unsigned int CallCraftCost(uintptr_t fnAddr, void* panel, void* item)`
- L290 `float ReadSkillFactor(void* panel, bool medical)`: The panel's ctor-cached skill factor.
- L305 `bool FireActivate(void* control)`
- L320 `bool ResolveRowCommit(void* panel, void* control, void** outListBox)`
- L337 `void DispatchRowCommit(void* panel, void* listBox, void* row)`
- L387 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L445 `void* ViewToggleButton(void* panel)`
- L451 `void* CommitButton(void* panel)`
- L459 `bool ToggleViewFromHotkey()`: Q / E — flip create <-> break down.
- L482 `void SyncSelectedRowFromChainFocus()`
- L507 `bool IsHiddenCraftingListBox(void* panel, void* listBox)`
- L518 `bool OwnsPanelTitle(void* panel)`
- L523 `void Tick()`
