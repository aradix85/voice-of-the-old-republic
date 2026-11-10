# menus_chain_input_enter.cpp (243 lines)

Source: `patches/Accessibility/menus_chain_input_enter.cpp`

Enter on the chain cursor: the per-panel activation routes and the generic activate.

## Declarations (in source order)

- L45 `bool LevelUpStepDisabled(PanelKind kind, const ChainEntry& e)`: The level-up wizard enables only the current step (bit_flags bit 3, from CSWGuiControl::SetEnabled); a mouse click is inert on the others.
- L60 `bool StoreItemRow(PanelKind, const ChainEntry& e)`: The generic activate on a store row only refreshes the description; the trade goes through OnControlInvAButton / OnControlStoreAButton.
- L73 `bool CraftRow(PanelKind, const ChainEntry& e)`: K2 workbench and crafting list rows: select the row, then fire the panel's commit button (BTN_UPGRADEITEMS, BTN_Accept), as one deferred op.
- L84 `bool JournalRow(PanelKind kind, const ChainEntry& e)`: Journal rows have no activate of their own: Enter reads the description a sighted player sees on hover.
- L95 `bool QuestItemRow(PanelKind kind, const ChainEntry& e)`: Quest-item rows likewise read the item's description; BTN_BACK is no item row and takes the generic activate.
- L106 `bool TextOnly(PanelKind, const ChainEntry& e)`: Modal body text has nothing to activate: say it again.
- L115 `bool TabButton(PanelKind, const ChainEntry& e)`: Tab buttons switch only through the mouse path.
- L129 `bool EquipSlot(PanelKind kind, const ChainEntry& e)`: Equip slot buttons: the generic activate takes the keyboard-shortcut path, which pops "no items" instead of filling LB_ITEMS.
- L145 `bool WorkbenchUpgradeSlot(PanelKind kind, const ChainEntry& e)`: Workbench upgrade slots, the same way: CSWGuiUpgrade::OnEnterSlot plus OnSlotSelected build the list of compatible mods into LB_ITEMS.
- L161 `bool WagerStepButton(PanelKind kind, const ChainEntry& e)`: The Pazaak wager popup's less/more buttons act only on their push callback (the panel's HandleInputEvent 0x2f/0x30), not on the generic activate.
- L180 `bool PartyAddBlocked(PanelKind kind, const ChainEntry& e)`: Adding a companion to a full party: the engine's toggle refuses silently, so it still runs (for its click sound) and "Party full" says why.
- L205 `void HandleEnterActivation(void* activePanel, int code, int val, bool& consumed)`
