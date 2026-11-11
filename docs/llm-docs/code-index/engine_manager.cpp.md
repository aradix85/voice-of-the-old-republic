# engine_manager.cpp (188 lines)

Source: `patches/Accessibility/engine_manager.cpp`

(From engine_manager.h.) CSWGuiManager surface — singleton lookup, panels[]/modal_stack walks, cursor + click-sim primitives.

Pure read + raw PFN typedefs. Constants and PFN at file scope (matches engine_input.h / engine_offsets.h) so callsites read clean (`*kAddrGuiManagerPtr`, reinterpret_cast<PFN_*>(...)).

GoG bytes match Steam — addresses lift directly from Lane's DB.

## Declarations (in source order)

- L14 `void* GetGuiManager()`
- L22 `int ReadPanelList(void* mgr, size_t dataOffset, size_t sizeOffset, void** out, int maxEntries, int* outRawCount)`: Both arrays are CExoArrayList-shaped (data, size, capacity) and differ only in their base offsets, so one body serves panels[] and modal_stack.
- L42 `int ReadPanelArray(void* mgr, void** outPanels, int maxEntries, int* outRawCount)`
- L48 `int ReadModalStack(void* mgr, void** outModal, int maxEntries, int* outRawCount)`
- L64 `static bool IsTransparentForegroundKind(PanelKind k)`: Panels that exist in the engine's panels[] for rendering reasons but never receive input.
- L73 `bool IsPanelInManager(void* panel)`
- L91 `void* FindOwningPanel(void* control)`
- L124 `void* GetForegroundPanel(void* mgr)`: NOT migrated to ReadPanelArray, deliberately.
- L159 `void LogManagerStack(void* mgr, const char* tag)`
