# engine_manager.cpp (238 lines)

Source: `patches/Accessibility/engine_manager.cpp`

(From engine_manager.h.) CSWGuiManager surface — singleton lookup, panels[]/modal_stack walks, cursor + click-sim primitives.

Pure read + raw PFN typedefs. Constants and PFN at file scope (matches engine_input.h / engine_offsets.h) so callsites read clean (`*kAddrGuiManagerPtr`, reinterpret_cast<PFN_*>(...)).

GoG bytes match Steam — addresses lift directly from Lane's DB.

## Declarations (in source order)

- L15 `void* GetGuiManager()`
- L23 `int ReadPanelList(void* mgr, size_t dataOffset, size_t sizeOffset, void** out, int maxEntries, int* outRawCount)`: Both arrays are CExoArrayList-shaped (data, size, capacity) and differ only in their base offsets, so one body serves panels[] and modal_stack.
- L43 `int ReadPanelArray(void* mgr, void** outPanels, int maxEntries, int* outRawCount)`
- L49 `int ReadModalStack(void* mgr, void** outModal, int maxEntries, int* outRawCount)`
- L65 `static bool IsTransparentForegroundKind(PanelKind k)`: Panels that exist in the engine's panels[] for rendering reasons but never receive input.
- L74 `bool IsPanelInManager(void* panel)`
- L92 `void* FindOwningPanel(void* control)`
- L125 `void* GetForegroundPanel(void* mgr)`: NOT migrated to ReadPanelArray, deliberately.
- L164 `static bool OpensOnlyAsModal(PanelKind k)`: The kinds the engine only ever shows on the modal stack.
- L168 `void* FindOrphanedModalPanel(void* mgr)`
- L200 `bool PushModalPanel(void* mgr, void* panel)`
- L209 `void LogManagerStack(void* mgr, const char* tag)`
