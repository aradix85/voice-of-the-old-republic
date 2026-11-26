# menus_internal.h (324 lines)

Source: `patches/Accessibility/menus_internal.h`

The menus feature's private seam: helpers and state the menus*.cpp files share, plus the panel-member resolvers peek_description borrows. The public interface is menus.h.

## Declarations (in source order)

- L19 `bool IsChainNavigable(void* control)`
- L20 `bool IsClassSelectionIcon(void* panel, void* control)`
- L29 `bool IsItemListBox(void* control)`: True for a list box that holds things to choose: more than one row, or a single row that can itself be chosen (a button, a toggle, a slider).
- L32 `const char* ClassLabelCacheLookup(void* panel, void* icon)`: Chargen class selection: the (panel, icon) to spoken text cache.
- L33 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
- L38 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose "class name" + the engine's class blurb into one announce string.
- L46 `void ClassDescBaselineCapture(void* panel, const char* text)`: Class-description placeholder discrimination.
- L47 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L58 `int ScaleGuiThresholdPx(int guiPx)`: Convert a distance measured in .gui authoring units into the units GetControlCenter returns on the running game.
- L62 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: Screen centre of a control's extent; false on a degenerate extent or a freed control.
- L65 `void* FindControlById(void* panel, int id)`: A child control of `panel` by its .gui id (the +0x50 field).
- L68 `bool IsSaveLoadPanel(void* panel)`: The CSWGuiSaveLoad panel, by its vtable.
- L71 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: A CExoString field of a save-row entry, by byte offset; nullptr when empty.
- L78 `struct ListBoxNavResult`: Result of a single arrow-key step on a listbox cursor.
- L89 `enum class ListBoxNavOp`: Direction of cursor motion driven by DriveListBoxSelection.
- L98 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Drive a CSWGuiListBox cursor in response to Nav-Up / Nav-Down / Home / End.
- L110 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Same contract as DriveListBoxSelection, but drives the engine's own CSWGuiListBox::SetSelectedControl instead of writing selection_index raw.
- L116 `int DrivenListBoxSelection(void* listbox)`: The row the keyboard holds in `listbox`: the one DriveListBoxSelectionEngine last selected there, or the engine's selection_index when the keyboard has not driven this list since it was filled.
- L121 `void ForgetDrivenListBoxSelection()`: Drop the row DriveListBoxSelectionEngine remembers.
- L133 `bool ParkCursorToCorner(const char* tag)`: Warp the OS cursor to the empty top-left corner of the screen so the engine's per-frame hover-select (CSWGuiListBox::HandleMouseMove -> HitCheckMouseLocal -> SetSelectedControl) finds no row under it and stops overwri...
- L139 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control resolved through a panel-member resolver.
- L160 `void* CurrentPanelIfLive()`: Liveness-filtered view of g_currentPanel — returns it only while it is still in the manager's panels[], nullptr once the engine has freed it.
- L193 `int EquipSlotIndexFromButton(void* panel, void* control)`: Equip-panel slot identity from the engine's embedded control arrays (kEquipPanelSlotButtons/LabelsOffset).
- L194 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L197 `inline int EquipSlotCount()`: Slots in the engine's arrays for the running game (9 on K1, 11 on K2).
- L204 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Slot-button membership on the workbench upgrade panel via its embedded button run (K1: 7 from +0x64, K2: 9 from +0x7a8).
- L215 `void* EquipPanelItemsListBox(void* panel)`: The equipment screen (equip / equip_p).
- L216 `void* EquipPanelEquipButton(void* panel)`
- L217 `void* EquipPanelBackButton(void* panel)`
- L220 `void* UpgradePanelItemsListBox(void* panel)`: CSWGuiUpgrade, the workbench upgrade screen (upgrade / upgrade_p).
- L221 `void* UpgradePanelAssembleButton(void* panel)`
- L222 `void* UpgradePanelBackButton(void* panel)`
- L225 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad (saveload / saveload_p).
- L226 `void* SaveLoadPanelActionButton(void* panel)`
- L227 `void* SaveLoadPanelBackButton(void* panel)`
- L234 `void* UpgradeSelPanelListBox(void* panel)`: KOTOR 2 workbench and crafting; nullptr on KOTOR 1.
- L239 `bool IsUpgradePickerNoneRow(void* row)`: KOTOR 2: `row` is the upgrade picker's "-" entry (none / remove).
- L240 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L241 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L242 `void* CraftPanelShopListBox(void* panel)`
- L243 `void* CraftPanelInvListBox(void* panel)`
- L244 `void* CraftPanelAcceptButton(void* panel)`
- L245 `void* CraftPanelExamineButton(void* panel)`
- L246 `void* CraftPanelTitleLabel(void* panel)`
- L249 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp (pwrlvlup / pwrlvlup_p).
- L250 `void* PowersPanelDescListBox(void* panel)`
- L251 `void* PowersPanelPowerLabel(void* panel)`
- L252 `void* PowersPanelSubTitleLabel(void* panel)`
- L253 `void* PowersPanelRecommendedButton(void* panel)`
- L254 `void* PowersPanelAcceptButton(void* panel)`
- L255 `void* PowersPanelBackButton(void* panel)`
- L258 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer (container / container_p).
- L259 `void* ContainerPanelOkButton(void* panel)`
- L260 `void* ContainerPanelCancelButton(void* panel)`
- L261 `void* ContainerPanelGiveItemsButton(void* panel)`
- L264 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen (ftchrgen / ftchrgen_p).
- L265 `void* FeatsPanelBackButton(void* panel)`
- L266 `void* FeatsPanelRecommendedButton(void* panel)`
- L270 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox (skillinfo / skillinfo_p), the granted-feats / level-up info popup.
- L271 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L272 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L276 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect (ScriptSelect.gui), the character sheet's combat-behaviour picker; KOTOR 1 only.
- L277 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L278 `void* ScriptSelectPanelBackButton(void* panel)`
- L281 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup (pazaakwager / pazaakwager_p), the pazaak bet popup.
- L282 `void* WagerPopupPanelMoreButton(void* panel)`
- L283 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L286 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart (pazaaksetup / pazaaksetup_p), the side-deck builder.
- L287 `void* PazaakStartPanelClearButton(void* panel)`
- L292 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings (OPTKeyMapping / OPTKeyMapping_p), the Keymapping screen.
- L293 `void* KeyMapPanelDefaultButton(void* panel)`
- L294 `void* KeyMapPanelAcceptButton(void* panel)`
- L295 `void* KeyMapPanelCancelButton(void* panel)`
- L296 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L300 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p), the workbench per-category item picker.
- L301 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L302 `void* WorkbenchItemsPanelBackButton(void* panel)`

Also 8 constants or variables at namespace scope.
