# peek_description.cpp (307 lines)

Source: `patches/Accessibility/peek_description.cpp`

Shift+Up/Down description peek: the panel descriptions (inventory, journal, abilities, store, galaxy map, any control's tooltip) and the dispatch to the item-description paths in peek_description_item.cpp.

## Declarations (in source order)

- L39 `void RefreshInventory(void* panel, void* focused)`
- L45 `void RefreshStore(void* panel, void* focused)`
- L51 `void RefreshJournal(void* panel, void* focused)`
- L67 `void RefreshAbilities(void* panel, void* /*focused*/)`: Repaint LB_DESC for the focused entry before Shift+Up/Down pages it.
- L92 `const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k)`
- L103 `int StepBlockCursor(bool down, int count)`
- L120 `void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`: Several OnControlEntered overrides early-out on `entry->is_active != 0`.
- L143 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`: Rows can be CSWGuiLabel OR CSWGuiButton (engine uses both for text-only containers).
- L167 `void OnShiftReleased()`
- L183 `bool IsChainItemRow(acc::engine::PanelKind kind, void* panel, void* focused)`: Chain-navigated rows whose focused control is the item entry itself.
- L195 `bool SpeakControlTooltip(acc::engine::PanelKind kind, void* focused)`: Fallback for panels without a description listbox: the focused control's own .gui tooltip (LevelUp, PartySelection, options widgets and the like).
- L210 `void PageDescriptionListBox(acc::engine::PanelKind kind, const PanelPeekInfo& info, void* panel, void* focused, bool down)`: Re-stage the panel's description for the focused row, then page its rows.
- L246 `bool HandleShiftArrow(int param_1, int param_2, void* activePanel, void* focusedControl)`
