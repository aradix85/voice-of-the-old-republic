# engine_manager.cpp (191 lines)

Source: `patches/Accessibility/engine_manager.cpp`

(From engine_manager.h.) CSWGuiManager surface — singleton lookup, panels[]/modal_stack walks, cursor + click-sim primitives.

Pure read + raw PFN typedefs. Constants and PFN at file scope (matches engine_input.h / engine_offsets.h) so callsites read clean (`*kAddrGuiManagerPtr`, reinterpret_cast<PFN_*>(...)).

GoG bytes match Steam — addresses lift directly from Lane's DB.

## Declarations (in source order)

- L13 `void* GetGuiManager()`
- L25 `int ReadPanelList(void* mgr, size_t dataOffset, size_t sizeOffset, void** out, int maxEntries, int* outRawCount)`: Both arrays are CExoArrayList-shaped (data, size, capacity) and differ only in their base offsets, so one body serves panels[] and modal_stack.
- L45 `int ReadPanelArray(void* mgr, void** outPanels, int maxEntries, int* outRawCount)`
- L51 `int ReadModalStack(void* mgr, void** outModal, int maxEntries, int* outRawCount)`
- L67 `static bool IsTransparentForegroundKind(PanelKind k)`: Panels that exist in the engine's panels[] for rendering reasons but never receive input.
- L76 `bool IsPanelInManager(void* panel)`
- L94 `void* FindOwningPanel(void* control)`
- L127 `void* GetForegroundPanel(void* mgr)`: NOT migrated to ReadPanelArray, deliberately.
- L162 `void LogManagerStack(void* mgr, const char* tag)`
