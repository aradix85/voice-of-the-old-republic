# menus_internal_listbox.cpp (191 lines)

Source: `patches/Accessibility/menus_internal_listbox.cpp`

Driving a listbox's selection from the keyboard: our own selection write, and the engine's own step for the screens that need it.

## Declarations (in source order)

- L33 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L55 `static bool DriveListBoxSelectionBody(void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`: The body, split out so the SEH wrapper above stays free of anything needing unwinding (MSVC C2712) — see the CLAUDE.md note on __try and objects.
- L117 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L135 `static bool DriveListBoxSelectionEngineBody( void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`
