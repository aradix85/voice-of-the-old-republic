# peek_description.cpp (308 lines)

Source: `patches/Accessibility/peek_description.cpp`

Shift+Up/Down description peek: the panel descriptions (inventory, journal, abilities, store, galaxy map, any control's tooltip) and the dispatch to the item-description paths in peek_description_item.cpp.

## Declarations (in source order)

- L38 `void RefreshInventory(void* panel, void* focused)`
- L44 `void RefreshStore(void* panel, void* focused)`
- L50 `void RefreshJournal(void* panel, void* focused)`
- L66 `void RefreshAbilities(void* panel, void* /*focused*/)`: Repaint LB_DESC for the focused entry before Shift+Up/Down pages it.
- L91 `const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k)`
- L102 `int StepBlockCursor(bool down, int count)`
- L119 `void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused)`: Several OnControlEntered overrides early-out on `entry->is_active != 0`.
- L142 `const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize)`: Rows can be CSWGuiLabel OR CSWGuiButton (engine uses both for text-only containers).
- L166 `void OnShiftReleased()`
- L182 `bool IsChainItemRow(acc::engine::PanelKind kind, void* panel, void* focused)`: Chain-navigated rows whose focused control is the item entry itself.
- L194 `bool SpeakControlTooltip(acc::engine::PanelKind kind, void* focused)`: Fallback for panels without a description listbox: the focused control's own .gui tooltip (LevelUp, PartySelection, options widgets and the like).
- L209 `void PageDescriptionListBox(acc::engine::PanelKind kind, const PanelPeekInfo& info, void* panel, void* focused, bool down)`: Re-stage the panel's description for the focused row, then page its rows.
- L247 `bool HandleShiftArrow(int param_1, int param_2, void* activePanel, void* focusedControl)`
