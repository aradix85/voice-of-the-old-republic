# menus_pending_activate.cpp (210 lines)

Source: `patches/Accessibility/menus_pending_activate.cpp`

Activating a control by hand (its HandleInputEvent), with the guards and repairs around it: a stale chain panel, a chargen sub-screen closing, a button that frees itself, and the journal and inventory lists the activation rebuilds.

## Declarations (in source order)

- L36 `bool ClosesChargenSubScreen(void* target, PanelKind kindAtDispatch)`: True when `target` is the Accept or Cancel button of a chargen sub-screen of InGameLevelUp, i.e.
- L80 `void ForgetSelfDestroyingButton(void* target)`: Self-destroying-button hazard: Accept on InGameLevelUp commits the level-up AND pops the panel, freeing the button before the handler returns.
- L101 `void RepairJournalList(void* target)`: Journal Sort/Swap repair.
- L123 `void RepairInventoryFilter(void* target)`: Inventory filter repair — the journal-Sort case exactly.
- L145 `void DrainActivate(const PendingOp& op)`: Direct activate via HandleInputEvent(0x27, 1).
