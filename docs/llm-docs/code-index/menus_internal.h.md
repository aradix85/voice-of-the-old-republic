# menus_internal.h (304 lines)

Source: `patches/Accessibility/menus_internal.h`

The menus feature's private seam: helpers and state the menus*.cpp files share, plus the panel-member resolvers peek_description borrows. The public interface is menus.h.

## Declarations (in source order)

- L19 `bool IsChainNavigable(void* control)`
- L20 `bool IsClassSelectionIcon(void* panel, void* control)`
- L25 `bool IsMultiRowListBox(void* control)`: True for a listbox container with more than one row.
- L28 `const char* ClassLabelCacheLookup(void* panel, void* icon)`: Chargen class selection: the (panel, icon) to spoken text cache.
- L29 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
- L34 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose "class name" + the engine's class blurb into one announce string.
- L42 `void ClassDescBaselineCapture(void* panel, const char* text)`: Class-description placeholder discrimination.
- L43 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L54 `int ScaleGuiThresholdPx(int guiPx)`: Convert a distance measured in .gui authoring units into the units GetControlCenter returns on the running game.
- L58 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: Screen centre of a control's extent; false on a degenerate extent or a freed control.
- L61 `void* FindControlById(void* panel, int id)`: A child control of `panel` by its .gui id (the +0x50 field).
- L64 `bool IsSaveLoadPanel(void* panel)`: The CSWGuiSaveLoad panel, by its vtable.
- L67 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: A CExoString field of a save-row entry, by byte offset; nullptr when empty.
- L74 `struct ListBoxNavResult`: Result of a single arrow-key step on a listbox cursor.
- L85 `enum class ListBoxNavOp`: Direction of cursor motion driven by DriveListBoxSelection.
- L94 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Drive a CSWGuiListBox cursor in response to Nav-Up / Nav-Down / Home / End.
- L105 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Same contract as DriveListBoxSelection, but drives the engine's own CSWGuiListBox::SetSelectedControl instead of writing selection_index raw.
- L118 `bool ParkCursorToCorner(const char* tag)`: Warp the OS cursor to the empty top-left corner of the screen so the engine's per-frame hover-select (CSWGuiListBox::HandleMouseMove -> HitCheckMouseLocal -> SetSelectedControl) finds no row under it and stops overwri...
- L124 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control resolved through a panel-member resolver.
- L145 `void* CurrentPanelIfLive()`: Liveness-filtered view of g_currentPanel — returns it only while it is still in the manager's panels[], nullptr once the engine has freed it.
- L178 `int EquipSlotIndexFromButton(void* panel, void* control)`: Equip-panel slot identity from the engine's embedded control arrays (kEquipPanelSlotButtons/LabelsOffset).
- L179 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L182 `inline int EquipSlotCount()`: Slots in the engine's arrays for the running game (9 on K1, 11 on K2).
- L189 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Slot-button membership on the workbench upgrade panel via its embedded button run (K1: 7 from +0x64, K2: 9 from +0x7a8).
- L200 `void* EquipPanelItemsListBox(void* panel)`: The equipment screen (equip / equip_p).
- L201 `void* EquipPanelEquipButton(void* panel)`
- L202 `void* EquipPanelBackButton(void* panel)`
- L205 `void* UpgradePanelItemsListBox(void* panel)`: CSWGuiUpgrade, the workbench upgrade screen (upgrade / upgrade_p).
- L206 `void* UpgradePanelAssembleButton(void* panel)`
- L207 `void* UpgradePanelBackButton(void* panel)`
- L210 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad (saveload / saveload_p).
- L211 `void* SaveLoadPanelActionButton(void* panel)`
- L212 `void* SaveLoadPanelBackButton(void* panel)`
- L219 `void* UpgradeSelPanelListBox(void* panel)`: KOTOR 2 workbench and crafting; nullptr on KOTOR 1.
- L220 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L221 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L222 `void* CraftPanelShopListBox(void* panel)`
- L223 `void* CraftPanelInvListBox(void* panel)`
- L224 `void* CraftPanelAcceptButton(void* panel)`
- L225 `void* CraftPanelExamineButton(void* panel)`
- L226 `void* CraftPanelTitleLabel(void* panel)`
- L229 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp (pwrlvlup / pwrlvlup_p).
- L230 `void* PowersPanelDescListBox(void* panel)`
- L231 `void* PowersPanelPowerLabel(void* panel)`
- L232 `void* PowersPanelSubTitleLabel(void* panel)`
- L233 `void* PowersPanelRecommendedButton(void* panel)`
- L234 `void* PowersPanelAcceptButton(void* panel)`
- L235 `void* PowersPanelBackButton(void* panel)`
- L238 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer (container / container_p).
- L239 `void* ContainerPanelOkButton(void* panel)`
- L240 `void* ContainerPanelCancelButton(void* panel)`
- L241 `void* ContainerPanelGiveItemsButton(void* panel)`
- L244 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen (ftchrgen / ftchrgen_p).
- L245 `void* FeatsPanelBackButton(void* panel)`
- L246 `void* FeatsPanelRecommendedButton(void* panel)`
- L250 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox (skillinfo / skillinfo_p), the granted-feats / level-up info popup.
- L251 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L252 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L256 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect (ScriptSelect.gui), the character sheet's combat-behaviour picker; KOTOR 1 only.
- L257 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L258 `void* ScriptSelectPanelBackButton(void* panel)`
- L261 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup (pazaakwager / pazaakwager_p), the pazaak bet popup.
- L262 `void* WagerPopupPanelMoreButton(void* panel)`
- L263 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L266 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart (pazaaksetup / pazaaksetup_p), the side-deck builder.
- L267 `void* PazaakStartPanelClearButton(void* panel)`
- L272 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings (OPTKeyMapping / OPTKeyMapping_p), the Keymapping screen.
- L273 `void* KeyMapPanelDefaultButton(void* panel)`
- L274 `void* KeyMapPanelAcceptButton(void* panel)`
- L275 `void* KeyMapPanelCancelButton(void* panel)`
- L276 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L280 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p), the workbench per-category item picker.
- L281 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L282 `void* WorkbenchItemsPanelBackButton(void* panel)`

Also 8 constants or variables at namespace scope.
