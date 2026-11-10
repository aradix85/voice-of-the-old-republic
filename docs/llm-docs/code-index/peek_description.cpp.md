# peek_description.cpp (377 lines)

Source: `patches/Accessibility/peek_description.cpp`

(From peek_description.h.) Shift+Up/Down description peek.

Shift+arrow speaks one description block at a time from the focused panel's description listbox (Inventory, Journal, Abilities, Store) or from per-item tooltip resolution (Container, Equip picker, Workbench) or the focused control's tooltip strref as a generic fallback. Shift release resets the block cursor.

Adding a new panel = one entry in the panel registry inside the .cpp (offset of the description_listbox member, optional refresh adapter).

## Declarations (in source order)

- L40 `static void RefreshInventory(void* panel, void* focused)`
- L46 `static void RefreshStore(void* panel, void* focused)`
- L52 `static void RefreshJournal(void* panel, void* focused)`
- L68 `static void RefreshAbilities(void* panel, void* /*focused*/)`: Repaint LB_DESC for the focused entry before Shift+Up/Down pages it.
- L107 `const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k)`
- L114 `bool ShiftHeld()`
- L126 `void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`: Several OnControlEntered overrides early-out on `entry->is_active != 0`.
- L151 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`: Rows can be CSWGuiLabel OR CSWGuiButton (engine uses both for text-only containers).
- L175 `void OnShiftReleased()`
- L187 `bool HandleShiftArrow(int param_1, int param_2, void* activePanel, void* focusedControl)`
