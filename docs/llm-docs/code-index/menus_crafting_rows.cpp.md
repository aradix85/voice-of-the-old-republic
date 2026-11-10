# menus_crafting_rows.cpp (326 lines)

Source: `patches/Accessibility/menus_crafting_rows.cpp`

Rows of the K2 workbench and crafting lists: the row commit (selection, is_active and the panel's own commit button), the cost or yield suffix on a chain step, and the chain focus mirrored into the engine selection.

## Declarations (in source order)

- L40 `int RowIndexIn(void* listBox, void* row)`: Index of `row` in the listbox's controls array, or -1.
- L56 `bool WriteListBoxSelection(void* listBox, int index)`
- L90 `bool SelectRow(void* listBox, void* row, int index)`: Make `row` the engine's selected row of `listBox`: what a mouse click on the row leaves behind, minus the click.
- L102 `void* ResolveRowItem(void* row)`: The CSWSItem* behind a crafting row, or null.
- L118 `bool IsRowCraftable(void* row)`: Can the character actually make (or break down) this row?
- L133 `unsigned int CallCraftCost(uintptr_t fnAddr, void* panel, void* item)`
- L145 `float ReadSkillFactor(void* panel, bool medical)`: The panel's ctor-cached skill factor.
- L160 `bool FireActivate(void* control)`
- L175 `bool ResolveRowCommit(void* panel, void* control, void** outListBox)`
- L192 `void DispatchRowCommit(void* panel, void* listBox, void* row)`
- L242 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L300 `void SyncSelectedRowFromChainFocus()`
