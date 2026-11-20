# menus_internal_listbox.cpp (153 lines)

Source: `patches/Accessibility/menus_internal_listbox.cpp`

Driving a listbox's selection from the keyboard, by our own selection write or by the engine's SetSelectedControl, and the row the keyboard holds in an engine-driven list.

## Declarations (in source order)

- L42 `bool DrivenRowIsOf(void* listbox, int rowCount)`
- L50 `short TargetRow(ListBoxNavOp op, short oldSel, short minSel, int rowCount)`: The row a step lands on, clamped without wrapping.
- L63 `bool DriveBody(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine, ListBoxNavResult& out)`: The step itself, outside the SEH frame so that frame holds nothing that needs unwinding (MSVC C2712).
- L116 `bool DriveGuarded(void* listbox, ListBoxNavOp op, short minSel, bool viaEngine, ListBoxNavResult& out)`: A stale or unported listbox pointer declines the step instead of taking the process down; the engine path would otherwise also hand a wild `this` to SetSelectedControl.
- L130 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L135 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L140 `int DrivenListBoxSelection(void* listbox)`
- L148 `void ForgetDrivenListBoxSelection()`
