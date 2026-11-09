# engine_manager.h (162 lines)

Source: `patches/Accessibility/engine_manager.h`

CSWGuiManager surface — singleton lookup, panels[]/modal_stack walks, cursor + click-sim primitives.

Pure read + raw PFN typedefs. Constants and PFN at file scope (matches engine_input.h / engine_offsets.h) so callsites read clean (`*kAddrGuiManagerPtr`, reinterpret_cast<PFN_*>(...)).

GoG bytes match Steam — addresses lift directly from Lane's DB.

## Declarations (in source order)

- L21 `void* GetGuiManager()`: *kAddrGuiManagerPtr, SEH-guarded.
- L49 `int ReadPanelArray(void* mgr, void** outPanels, int maxEntries, int* outRawCount = nullptr)`: Snapshot the manager's panels[] / modal_stack into a caller-owned buffer.
- L51 `int ReadModalStack(void* mgr, void** outModal, int maxEntries, int* outRawCount = nullptr)`
- L58 `void* FindOwningPanel(void* control)`: Fallback when caller doesn't pass owner explicitly — g_currentPanel only updates on SetActiveControl; chain rebinds can land here while it points at a previous owner.
- L65 `bool IsPanelInManager(void* panel)`: Pointer-equality scan over panels[].
- L69 `void* GetForegroundPanel(void* mgr)`: Topmost owner: modal_stack top if non-empty, else last panels[] entry (last-pushed draws on top), else nullptr.
- L73 `void LogManagerStack(void* mgr, const char* tag)`: Dumps panels + modal_stack + GetForegroundPanel.
- L137 `typedef void (__thiscall* PFN_MoveMouseToPosition)(void* gm, int x, int y)`
- L160 `typedef int (__thiscall* PFN_ManagerLMouseDown)(void* gm, int press)`
- L161 `typedef int (__thiscall* PFN_ManagerLMouseUp)(void* gm)`

Also 9 constants or variables at namespace scope.
