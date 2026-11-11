# peek_description_internal.h (51 lines)

Source: `patches/Accessibility/peek_description_internal.h`

Internal to the peek_description module (peek_description*.cpp); no other file includes it. The public interface is peek_description.h. Here: what the files share.

## Declarations (in source order)

- L28 `struct ItemTooltipPanelInfo`: Item-tooltip path for listbox-driven panels with no inline description listbox (Container, Equip picker), or where OnItemSelected would commit the action.
- L37 `int StepBlockCursor(bool down, int count)`: peek_description.cpp
- L38 `void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`
- L39 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`
- L42 `void ResetItemBlocks()`: peek_description_item.cpp
- L43 `bool SpeakItemBlocks(void* item, bool down)`
- L44 `bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down)`
- L45 `bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down)`
- L46 `const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k)`
- L47 `bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info, void* activePanel, bool down)`

Also 2 constants or variables at namespace scope.
