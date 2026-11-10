# menus_internal_members.cpp (475 lines)

Source: `patches/Accessibility/menus_internal_members.cpp`

The panels' controls by their ctor-bound members (equipment, workbench, save/load, crafting, powers, container, feats and the rest), with the .gui id kept as a tripwire, and the slot index of an equipment or upgrade slot button.

## Declarations (in source order)

- L21 `int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff, size_t stride)`
- L44 `void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId, const char* tag)`
- L68 `void* CraftMemberWithTripwire(void* panel, size_t componentOff, size_t chemicalOff, int guiId, const char* tag)`: The two crafting classes lay the same controls out at different offsets; pick per panel kind, or return nullptr when `panel` is neither.
- L101 `inline int SaveLoadLbGamesId()`: CSWGuiSaveLoad control ids from saveload.gui / K2 saveload_p.gui (verified against chain logs: patch-20260505-160124.log lines 45-65; K2 witnessed live in patch-20260813-212757 — K2 re-authored the screen and every id...
- L103 `inline int SaveLoadBtnBackId()`
- L105 `inline int SaveLoadBtnSaveLoadId()`
- L107 `int EquipSlotIndexFromButton(void* panel, void* control)`
- L113 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L121 `void* EquipPanelItemsListBox(void* panel)`
- L126 `void* EquipPanelEquipButton(void* panel)`
- L131 `void* EquipPanelBackButton(void* panel)`
- L136 `void* UpgradePanelItemsListBox(void* panel)`
- L142 `void* UpgradePanelAssembleButton(void* panel)`
- L150 `void* UpgradePanelBackButton(void* panel)`
- L160 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad — same engine-truth resolvers (members mined from both games' ctors, see kSaveLoadPanel*Offset).
- L165 `void* SaveLoadPanelActionButton(void* panel)`
- L171 `void* SaveLoadPanelBackButton(void* panel)`
- L176 `void* UpgradeSelPanelListBox(void* panel)`
- L182 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L188 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L193 `void* CraftPanelShopListBox(void* panel)`
- L199 `void* CraftPanelInvListBox(void* panel)`
- L205 `void* CraftPanelAcceptButton(void* panel)`
- L211 `void* CraftPanelExamineButton(void* panel)`
- L220 `void* CraftPanelTitleLabel(void* panel)`
- L234 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp resolvers (see menus_internal.h).
- L240 `void* PowersPanelDescListBox(void* panel)`
- L246 `void* PowersPanelPowerLabel(void* panel)`
- L252 `void* PowersPanelSubTitleLabel(void* panel)`
- L258 `void* PowersPanelRecommendedButton(void* panel)`
- L264 `void* PowersPanelAcceptButton(void* panel)`
- L270 `void* PowersPanelBackButton(void* panel)`
- L279 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer resolvers (see menus_internal.h).
- L284 `void* ContainerPanelOkButton(void* panel)`
- L289 `void* ContainerPanelCancelButton(void* panel)`
- L294 `void* ContainerPanelGiveItemsButton(void* panel)`
- L307 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen resolvers (see menus_internal.h).
- L313 `void* FeatsPanelBackButton(void* panel)`
- L319 `void* FeatsPanelRecommendedButton(void* panel)`
- L327 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox resolvers (see menus_internal.h).
- L332 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L337 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L345 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect resolvers (see menus_internal.h).
- L350 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L355 `void* ScriptSelectPanelBackButton(void* panel)`
- L362 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup resolvers (see menus_internal.h).
- L368 `void* WagerPopupPanelMoreButton(void* panel)`
- L374 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L381 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart resolvers (see menus_internal.h).
- L387 `void* PazaakStartPanelClearButton(void* panel)`
- L400 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings resolvers (see menus_internal.h).
- L405 `void* KeyMapPanelDefaultButton(void* panel)`
- L410 `void* KeyMapPanelAcceptButton(void* panel)`
- L415 `void* KeyMapPanelCancelButton(void* panel)`
- L420 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L439 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect resolvers (see menus_internal.h).
- L444 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L449 `void* WorkbenchItemsPanelBackButton(void* panel)`
- L459 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Workbench-upgrade slot membership via the panel's embedded button run (one contiguous array in both games — see the kUpgradePanelSlotButtons note in engine_offsets_fields.h).
