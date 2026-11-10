# menus_internal_listbox.cpp (100 lines)

Source: `patches/Accessibility/menus_internal_listbox.cpp`

Driving a listbox's selection from the keyboard, by our own selection write or by the engine's SetSelectedControl.

## Declarations (in source order)

- L19 `short TargetRow(ListBoxNavOp op, short oldSel, short minSel, int rowCount)`: The row a step lands on, clamped without wrapping.
- L32 `bool DriveBody(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine, ListBoxNavResult& out)`: The step itself, outside the SEH frame so that frame holds nothing that needs unwinding (MSVC C2712).
- L75 `bool DriveGuarded(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine, ListBoxNavResult& out)`: A stale or unported listbox pointer declines the step instead of taking the process down; the engine path would otherwise also hand a wild `this` to SetSelectedControl.
- L89 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L94 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
