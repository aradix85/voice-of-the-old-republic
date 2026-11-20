# menus_internal.h (320 lines)

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
- L106 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Same contract as DriveListBoxSelection, but drives the engine's own CSWGuiListBox::SetSelectedControl instead of writing selection_index raw.
- L112 `int DrivenListBoxSelection(void* listbox)`: The row the keyboard holds in `listbox`: the one DriveListBoxSelectionEngine last selected there, or the engine's selection_index when the keyboard has not driven this list since it was filled.
- L117 `void ForgetDrivenListBoxSelection()`: Drop the row DriveListBoxSelectionEngine remembers.
- L129 `bool ParkCursorToCorner(const char* tag)`: Warp the OS cursor to the empty top-left corner of the screen so the engine's per-frame hover-select (CSWGuiListBox::HandleMouseMove -> HitCheckMouseLocal -> SetSelectedControl) finds no row under it and stops overwri...
- L135 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control resolved through a panel-member resolver.
- L156 `void* CurrentPanelIfLive()`: Liveness-filtered view of g_currentPanel — returns it only while it is still in the manager's panels[], nullptr once the engine has freed it.
- L189 `int EquipSlotIndexFromButton(void* panel, void* control)`: Equip-panel slot identity from the engine's embedded control arrays (kEquipPanelSlotButtons/LabelsOffset).
- L190 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L193 `inline int EquipSlotCount()`: Slots in the engine's arrays for the running game (9 on K1, 11 on K2).
- L200 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Slot-button membership on the workbench upgrade panel via its embedded button run (K1: 7 from +0x64, K2: 9 from +0x7a8).
- L211 `void* EquipPanelItemsListBox(void* panel)`: The equipment screen (equip / equip_p).
- L212 `void* EquipPanelEquipButton(void* panel)`
- L213 `void* EquipPanelBackButton(void* panel)`
- L216 `void* UpgradePanelItemsListBox(void* panel)`: CSWGuiUpgrade, the workbench upgrade screen (upgrade / upgrade_p).
- L217 `void* UpgradePanelAssembleButton(void* panel)`
- L218 `void* UpgradePanelBackButton(void* panel)`
- L221 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad (saveload / saveload_p).
- L222 `void* SaveLoadPanelActionButton(void* panel)`
- L223 `void* SaveLoadPanelBackButton(void* panel)`
- L230 `void* UpgradeSelPanelListBox(void* panel)`: KOTOR 2 workbench and crafting; nullptr on KOTOR 1.
- L235 `bool IsUpgradePickerNoneRow(void* row)`: KOTOR 2: `row` is the upgrade picker's "-" entry (none / remove).
- L236 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L237 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L238 `void* CraftPanelShopListBox(void* panel)`
- L239 `void* CraftPanelInvListBox(void* panel)`
- L240 `void* CraftPanelAcceptButton(void* panel)`
- L241 `void* CraftPanelExamineButton(void* panel)`
- L242 `void* CraftPanelTitleLabel(void* panel)`
- L245 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp (pwrlvlup / pwrlvlup_p).
- L246 `void* PowersPanelDescListBox(void* panel)`
- L247 `void* PowersPanelPowerLabel(void* panel)`
- L248 `void* PowersPanelSubTitleLabel(void* panel)`
- L249 `void* PowersPanelRecommendedButton(void* panel)`
- L250 `void* PowersPanelAcceptButton(void* panel)`
- L251 `void* PowersPanelBackButton(void* panel)`
- L254 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer (container / container_p).
- L255 `void* ContainerPanelOkButton(void* panel)`
- L256 `void* ContainerPanelCancelButton(void* panel)`
- L257 `void* ContainerPanelGiveItemsButton(void* panel)`
- L260 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen (ftchrgen / ftchrgen_p).
- L261 `void* FeatsPanelBackButton(void* panel)`
- L262 `void* FeatsPanelRecommendedButton(void* panel)`
- L266 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox (skillinfo / skillinfo_p), the granted-feats / level-up info popup.
- L267 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L268 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L272 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect (ScriptSelect.gui), the character sheet's combat-behaviour picker; KOTOR 1 only.
- L273 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L274 `void* ScriptSelectPanelBackButton(void* panel)`
- L277 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup (pazaakwager / pazaakwager_p), the pazaak bet popup.
- L278 `void* WagerPopupPanelMoreButton(void* panel)`
- L279 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L282 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart (pazaaksetup / pazaaksetup_p), the side-deck builder.
- L283 `void* PazaakStartPanelClearButton(void* panel)`
- L288 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings (OPTKeyMapping / OPTKeyMapping_p), the Keymapping screen.
- L289 `void* KeyMapPanelDefaultButton(void* panel)`
- L290 `void* KeyMapPanelAcceptButton(void* panel)`
- L291 `void* KeyMapPanelCancelButton(void* panel)`
- L292 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L296 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p), the workbench per-category item picker.
- L297 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L298 `void* WorkbenchItemsPanelBackButton(void* panel)`

Also 8 constants or variables at namespace scope.
