# menus_internal.cpp (204 lines)

Source: `patches/Accessibility/menus_internal.cpp`

Implementations of the acc::menus::detail:: seam declared in menus_internal.h.

These helpers were defined in menus.cpp until the Phase-1 structure pass (refactoring candidate 1). menus_internal.h has always declared them as the "internal-but-exposed-across-TUs" contract shared by menus.cpp, menus_extract.cpp, menus_listbox.cpp and menus_keymap.cpp; this file is the matching .cpp every other seam header in this directory already has (menus_chain, menus_pending, menus_listbox, menus_editbox, menus_monitors).

Nothing here changed behaviourally in the move: the definitions are verbatim, including the saveload.gui control IDs and the chargen class-label cache, both of which are used only by functions in this file.

## Declarations (in source order)

- L34 `int ScaleGuiThresholdPx(int guiPx)`: See menus_internal.h for why this exists and when it is a no-op.
- L58 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: Center pixel of a control's hit area.
- L93 `bool IsChainNavigable(void* control)`: Screen-absolute center of a CSWGuiListBox row.
- L109 `void* FindControlById(void* panel, int id)`: Locate a child control on `panel` by its +0x50 ID field.
- L145 `bool IsSaveLoadPanel(void* panel)`: Detect the CSWGuiSaveLoad panel (the "Load Game" / "Save Game" dialog).
- L162 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: Read the user-visible text of a CExoString-style field on a control.
- L187 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control the caller resolved through an engine-member resolver.
