# menus_listbox.cpp (285 lines)

Source: `patches/Accessibility/menus_listbox.cpp`

listbox-driven panel input dispatcher.

See menus_listbox.h for the motivation. This file holds:

* The ListBoxPanelSpec struct (private) — one entry per panel kind. * All thirteen spec entries. Each carries ~5-6 small static callbacks that capture its quirks (announce format, button IDs, custom Enter dispatch, etc.). * The dispatcher TryHandleInput that walks the table. * The container monitor + the container give-mode key poll.

The two armed pickers (equipment, workbench upgrade) keep their SPECS here with the other eleven, but their mutable state and per-tick monitors live in menus_listbox_picker.cpp — see that file's header. The spec callbacks reach the state through the accessors in menus_listbox.h.

## Declarations (in source order)

- L71 `bool DispatchKeyDownEdge(const ListBoxPanelSpec& spec, void* panel, int param_1)`
- L165 `void LogStandard(int n, void* thisPtr, int param_1, int param_2, bool consumed)`
- L188 `void SpeakRowAndPosition(const ListBoxNavResult& r)`: The plain " , N of M" announce.
- L201 `bool TryHandleInput(int n, void* thisPtr, void* activePanel, int param_1, int param_2, int& outRv)`
- L252 `bool SetListBoxSelectedControl(void* listbox, int index)`: The engine's own selection commit — real row highlight, native scroll, and the onSelectionChanged work the preview labels depend on.
- L264 `void TickListboxMonitors()`
- L273 `const char* GetTitleOverride(void* panel)`
