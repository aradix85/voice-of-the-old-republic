# peek_description_internal.h (65 lines)

Source: `patches/Accessibility/peek_description_internal.h`

Internal to the peek_description module (peek_description*.cpp); no other file includes it. The public interface is peek_description.h. Here: what the files share.

## Declarations (in source order)

- L44 `struct ItemTooltipPanelInfo`: Item-tooltip path for listbox-driven panels with no inline description_listbox (Container, Equip picker) or where calling OnItemSelected would commit the action (Equip — picker resolves on the user's actual click).
- L53 `void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`: peek_description.cpp
- L54 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`
- L57 `bool SpeakItemBlocks(void* item, bool down)`: peek_description_item.cpp
- L58 `bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down)`
- L59 `bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down)`
- L60 `const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k)`
- L61 `void* ResolveRowItem(void* row)`
- L62 `bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info, void* activePanel, bool down)`

Also 6 constants or variables at namespace scope.
