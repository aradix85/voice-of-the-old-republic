# menus_internal_members.cpp (403 lines)

Source: `patches/Accessibility/menus_internal_members.cpp`

The panel-member resolvers (see menus_internal.h) and the slot index of an equipment or workbench-upgrade slot button.

## Declarations (in source order)

- L21 `int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff, size_t stride, int count)`: Index of `control` in the run of `count` controls embedded in `panel` at baseOff, or -1.
- L37 `void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId, const char* tag)`: guiId < 0 skips the tripwire, for an id that is ambiguous on the running game.
- L53 `void* CraftMemberWithTripwire(void* panel, size_t componentOff, size_t chemicalOff, int guiId, const char* tag)`: The member for whichever crafting class `panel` is, or nullptr.
- L66 `bool IsMedicalCraftPanel(void* panel)`
- L73 `int EquipSlotIndexFromButton(void* panel, void* control)`
- L80 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L89 `int UpgradeSlotIndexFromButton(void* panel, void* control)`
- L99 `void* EquipPanelItemsListBox(void* panel)`
- L105 `void* EquipPanelEquipButton(void* panel)`
- L111 `void* EquipPanelBackButton(void* panel)`
- L117 `void* UpgradePanelItemsListBox(void* panel)`
- L122 `void* UpgradePanelAssembleButton(void* panel)`
- L130 `void* UpgradePanelBackButton(void* panel)`
- L136 `void* SaveLoadPanelGamesListBox(void* panel)`
- L142 `void* SaveLoadPanelActionButton(void* panel)`
- L147 `void* SaveLoadPanelBackButton(void* panel)`
- L153 `void* UpgradeSelPanelListBox(void* panel)`
- L159 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L165 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L172 `void* CraftPanelShopListBox(void* panel)`: component_p and chemical_p agree on the listbox and accept ids but not on BTN_Examine and LBL_TITLE.
- L178 `void* CraftPanelInvListBox(void* panel)`
- L184 `void* CraftPanelAcceptButton(void* panel)`
- L190 `void* CraftPanelExamineButton(void* panel)`
- L197 `void* CraftPanelTitleLabel(void* panel)`
- L207 `void* PowersPanelPowersListBox(void* panel)`: The two games number pwrlvlup differently and collide: K1 id 6 is LB_POWERS where K2 id 6 is a label, K1 id 12 is BACK_BTN where K2 id 12 is LB_POWERS.
- L213 `void* PowersPanelDescListBox(void* panel)`
- L219 `void* PowersPanelPowerLabel(void* panel)`
- L225 `void* PowersPanelSubTitleLabel(void* panel)`
- L230 `void* PowersPanelRecommendedButton(void* panel)`
- L236 `void* PowersPanelAcceptButton(void* panel)`
- L242 `void* PowersPanelBackButton(void* panel)`
- L248 `void* ContainerPanelItemsListBox(void* panel)`
- L253 `void* ContainerPanelOkButton(void* panel)`
- L258 `void* ContainerPanelCancelButton(void* panel)`
- L263 `void* ContainerPanelGiveItemsButton(void* panel)`
- L271 `void* FeatsPanelAcceptButton(void* panel)`: ftchrgen: K1 9/11/12 = Recommended/Accept/Back, K2 9/10/11 = Back/Accept/Recommended.
- L277 `void* FeatsPanelBackButton(void* panel)`
- L283 `void* FeatsPanelRecommendedButton(void* panel)`
- L289 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`
- L294 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L299 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L304 `void* ScriptSelectPanelAiStateListBox(void* panel)`
- L309 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L314 `void* ScriptSelectPanelBackButton(void* panel)`
- L319 `void* WagerPopupPanelLessButton(void* panel)`
- L325 `void* WagerPopupPanelMoreButton(void* panel)`
- L331 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L337 `void* PazaakStartPanelPlayButton(void* panel)`
- L343 `void* PazaakStartPanelClearButton(void* panel)`
- L351 `void* KeyMapPanelEventListBox(void* panel)`: The ids are optkeymapping.gui's; KOTOR 2's file was never mined, so a renumbered optkeymapping_p shows up as mismatch lines, not a broken screen.
- L356 `void* KeyMapPanelDefaultButton(void* panel)`
- L361 `void* KeyMapPanelAcceptButton(void* panel)`
- L366 `void* KeyMapPanelCancelButton(void* panel)`
- L371 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L387 `void* WorkbenchItemsPanelItemsListBox(void* panel)`
- L392 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L397 `void* WorkbenchItemsPanelBackButton(void* panel)`
