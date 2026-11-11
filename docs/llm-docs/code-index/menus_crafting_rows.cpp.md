# menus_crafting_rows.cpp (226 lines)

Source: `patches/Accessibility/menus_crafting_rows.cpp`

Rows of the K2 workbench and crafting lists: the row commit (selection, is_active and the panel's own commit button), the cost or yield suffix on a chain step, and the chain focus mirrored into the engine selection.

## Declarations (in source order)

- L48 `bool SelectRow(void* listBox, void* row, int index)`: Makes `row` the selected row: what a mouse click leaves behind, minus the click.
- L62 `bool IsRowCraftable(void* row)`: The engine's own "can make this" gate; true when unreadable, so a bad read never invents a refusal.
- L73 `unsigned int CallCraftCost(uintptr_t fnAddr, void* panel, void* item)`: The engine's cost function (the two screens use different ones, see kAddrCraftComponentCost); 0 on a fault.
- L84 `float ReadSkillFactor(void* panel, bool medical)`: The panel's ctor-cached skill factor, or 0 for a fault or a value outside (0, 1]: a yield that cannot be trusted is not quoted.
- L95 `bool FireActivate(void* control)`: The control's own HandleInputEvent (vtable slot 15) with the activate code, as a click would.
- L109 `bool ResolveRowCommit(void* panel, void* control, void** outListBox)`
- L125 `void DispatchRowCommit(void* panel, void* listBox, void* row)`
- L162 `void AnnounceChainStepSuffix(void* panel, void* control)`
- L207 `void SyncSelectedRowFromChainFocus()`
