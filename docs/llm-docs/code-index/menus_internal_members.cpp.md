# menus_internal_members.cpp (406 lines)

Source: `patches/Accessibility/menus_internal_members.cpp`

The panel-member resolvers (see menus_internal.h) and the slot index of an equipment or workbench-upgrade slot button.

## Declarations (in source order)

- L22 `int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff, size_t stride, int count)`: Index of `control` in the run of `count` controls embedded in `panel` at baseOff, or -1.
- L38 `void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId, const char* tag)`: guiId < 0 skips the tripwire, for an id that is ambiguous on the running game.
- L54 `void* CraftMemberWithTripwire(void* panel, size_t componentOff, size_t chemicalOff, int guiId, const char* tag)`: The member for whichever crafting class `panel` is, or nullptr.
- L67 `bool IsMedicalCraftPanel(void* panel)`
- L74 `int EquipSlotIndexFromButton(void* panel, void* control)`
- L81 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L90 `int UpgradeSlotIndexFromButton(void* panel, void* control)`
- L100 `void* EquipPanelItemsListBox(void* panel)`
- L106 `void* EquipPanelEquipButton(void* panel)`
- L112 `void* EquipPanelBackButton(void* panel)`
- L118 `void* UpgradePanelItemsListBox(void* panel)`
- L123 `void* UpgradePanelAssembleButton(void* panel)`
- L131 `void* UpgradePanelBackButton(void* panel)`
- L137 `void* SaveLoadPanelGamesListBox(void* panel)`
- L143 `void* SaveLoadPanelActionButton(void* panel)`
- L148 `void* SaveLoadPanelBackButton(void* panel)`
- L154 `void* UpgradeSelPanelListBox(void* panel)`
- L160 `bool IsUpgradePickerNoneRow(void* row)`
- L166 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L172 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L179 `void* CraftPanelShopListBox(void* panel)`: component_p and chemical_p agree on the listbox and accept ids but not on BTN_Examine and LBL_TITLE.
- L185 `void* CraftPanelInvListBox(void* panel)`
- L191 `void* CraftPanelAcceptButton(void* panel)`
- L197 `void* CraftPanelExamineButton(void* panel)`
- L204 `void* CraftPanelTitleLabel(void* panel)`
- L214 `void* PowersPanelPowersListBox(void* panel)`: The two games number pwrlvlup differently and collide: K1 id 6 is LB_POWERS where K2 id 6 is a label, K1 id 12 is BACK_BTN where K2 id 12 is LB_POWERS.
- L220 `void* PowersPanelDescListBox(void* panel)`
- L226 `void* PowersPanelPowerLabel(void* panel)`
- L232 `void* PowersPanelSubTitleLabel(void* panel)`
- L237 `void* PowersPanelRecommendedButton(void* panel)`
- L243 `void* PowersPanelAcceptButton(void* panel)`
- L249 `void* PowersPanelBackButton(void* panel)`
- L255 `void* ContainerPanelItemsListBox(void* panel)`
- L260 `void* ContainerPanelOkButton(void* panel)`
- L265 `void* ContainerPanelCancelButton(void* panel)`
- L270 `void* ContainerPanelGiveItemsButton(void* panel)`
- L278 `void* FeatsPanelAcceptButton(void* panel)`: ftchrgen: K1 9/11/12 = Recommended/Accept/Back, K2 9/10/11 = Back/Accept/Recommended.
- L284 `void* FeatsPanelBackButton(void* panel)`
- L290 `void* FeatsPanelRecommendedButton(void* panel)`
- L296 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`
- L301 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L306 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L311 `void* ScriptSelectPanelAiStateListBox(void* panel)`
- L316 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L321 `void* ScriptSelectPanelBackButton(void* panel)`
- L326 `void* WagerPopupPanelLessButton(void* panel)`
- L332 `void* WagerPopupPanelMoreButton(void* panel)`
- L338 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L344 `void* PazaakStartPanelPlayButton(void* panel)`
- L350 `void* PazaakStartPanelClearButton(void* panel)`
- L358 `void* KeyMapPanelEventListBox(void* panel)`: The ids are optkeymapping.gui's; KOTOR 2's file was never mined, so a renumbered optkeymapping_p shows up as mismatch lines, not a broken screen.
- L363 `void* KeyMapPanelDefaultButton(void* panel)`
- L368 `void* KeyMapPanelAcceptButton(void* panel)`
- L373 `void* KeyMapPanelCancelButton(void* panel)`
- L378 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L390 `void* WorkbenchItemsPanelItemsListBox(void* panel)`
- L395 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L400 `void* WorkbenchItemsPanelBackButton(void* panel)`
