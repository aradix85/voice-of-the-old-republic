# peek_description.h (37 lines)

Source: `patches/Accessibility/peek_description.h`

Shift+Up/Down description peek.

Shift+arrow speaks one description block at a time from the focused panel's description listbox (Inventory, Journal, Abilities, Store) or from per-item tooltip resolution (Container, Equip picker, Workbench) or the focused control's tooltip strref as a generic fallback. Shift release resets the block cursor.

Adding a new panel = one entry in the panel registry inside the .cpp (offset of the description_listbox member, optional refresh adapter).

## Declarations (in source order)

- L23 `bool HandleShiftArrow(int param_1, int param_2, void* activePanel, void* focusedControl)`: Called from the manager-level input dispatch.
- L27 `void OnShiftReleased()`: Resets the block cursor; next Shift+arrow speaks block 0.
- L34 `bool SpeakItemRowDescription(void* row)`: Resolve a CSWGuiInGameItemEntry row to its item and speak the property description with interrupt=true.
