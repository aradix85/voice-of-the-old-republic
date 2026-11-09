# menus_internal.cpp (997 lines)

Source: `patches/Accessibility/menus_internal.cpp`

Implementations of the acc::menus::detail:: seam declared in menus_internal.h.

These helpers were defined in menus.cpp until the Phase-1 structure pass (refactoring candidate 1). menus_internal.h has always declared them as the "internal-but-exposed-across-TUs" contract shared by menus.cpp, menus_extract.cpp, menus_listbox.cpp and menus_keymap.cpp; this file is the matching .cpp every other seam header in this directory already has (menus_chain, menus_pending, menus_listbox, menus_editbox, menus_monitors).

Nothing here changed behaviourally in the move: the definitions are verbatim, including the saveload.gui control IDs and the chargen class-label cache, both of which are used only by functions in this file.

## Declarations (in source order)

- L50 `inline int SaveLoadLbGamesId()`: CSWGuiSaveLoad control ids from saveload.gui / K2 saveload_p.gui (verified against chain logs: patch-20260505-160124.log lines 45-65; K2 witnessed live in patch-20260813-212757 — K2 re-authored the screen and every id...
- L51 `inline int SaveLoadBtnBackId()`
- L52 `inline int SaveLoadBtnSaveLoadId()`
- L59 `int acc::menus::detail::ScaleGuiThresholdPx(int guiPx)`: See menus_internal.h for why this exists and when it is a no-op.
- L83 `bool acc::menus::detail::GetControlCenter(void* control, int& outCx, int& outCy)`: Center pixel of a control's hit area.
- L118 `bool acc::menus::detail::IsChainNavigable(void* control)`: Screen-absolute center of a CSWGuiListBox row.
- L134 `void* acc::menus::detail::FindControlById(void* panel, int id)`: Locate a child control on `panel` by its +0x50 ID field.
- L161 `int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff, size_t stride)`
- L176 `int acc::menus::detail::EquipSlotIndexFromButton(void* panel, void* control)`
- L182 `int acc::menus::detail::EquipSlotIndexFromControl(void* panel, void* control)`
- L199 `void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId, const char* tag)`
- L215 `void* acc::menus::detail::EquipPanelItemsListBox(void* panel)`
- L220 `void* acc::menus::detail::EquipPanelEquipButton(void* panel)`
- L225 `void* acc::menus::detail::EquipPanelBackButton(void* panel)`
- L230 `void* acc::menus::detail::UpgradePanelItemsListBox(void* panel)`
- L236 `void* acc::menus::detail::UpgradePanelAssembleButton(void* panel)`
- L244 `void* acc::menus::detail::UpgradePanelBackButton(void* panel)`
- L254 `void* acc::menus::detail::SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad — same engine-truth resolvers (members mined from both games' ctors, see kSaveLoadPanel*Offset).
- L259 `void* acc::menus::detail::SaveLoadPanelActionButton(void* panel)`
- L265 `void* acc::menus::detail::SaveLoadPanelBackButton(void* panel)`
- L279 `void* CraftMemberWithTripwire(void* panel, size_t componentOff, size_t chemicalOff, int guiId, const char* tag)`: The two crafting classes lay the same controls out at different offsets; pick per panel kind, or return nullptr when `panel` is neither.
- L293 `void* acc::menus::detail::UpgradeSelPanelListBox(void* panel)`
- L299 `void* acc::menus::detail::UpgradeSelPanelUpgradeButton(void* panel)`
- L305 `void* acc::menus::detail::UpgradeSelPanelTitleLabel(void* panel)`
- L310 `void* acc::menus::detail::CraftPanelShopListBox(void* panel)`
- L316 `void* acc::menus::detail::CraftPanelInvListBox(void* panel)`
- L322 `void* acc::menus::detail::CraftPanelAcceptButton(void* panel)`
- L328 `void* acc::menus::detail::CraftPanelExamineButton(void* panel)`
- L337 `void* acc::menus::detail::CraftPanelTitleLabel(void* panel)`
- L351 `void* acc::menus::detail::PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp resolvers (see menus_internal.h).
- L357 `void* acc::menus::detail::PowersPanelDescListBox(void* panel)`
- L363 `void* acc::menus::detail::PowersPanelPowerLabel(void* panel)`
- L369 `void* acc::menus::detail::PowersPanelSubTitleLabel(void* panel)`
- L375 `void* acc::menus::detail::PowersPanelRecommendedButton(void* panel)`
- L381 `void* acc::menus::detail::PowersPanelAcceptButton(void* panel)`
- L387 `void* acc::menus::detail::PowersPanelBackButton(void* panel)`
- L396 `void* acc::menus::detail::ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer resolvers (see menus_internal.h).
- L401 `void* acc::menus::detail::ContainerPanelOkButton(void* panel)`
- L406 `void* acc::menus::detail::ContainerPanelCancelButton(void* panel)`
- L411 `void* acc::menus::detail::ContainerPanelGiveItemsButton(void* panel)`
- L424 `void* acc::menus::detail::FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen resolvers (see menus_internal.h).
- L430 `void* acc::menus::detail::FeatsPanelBackButton(void* panel)`
- L436 `void* acc::menus::detail::FeatsPanelRecommendedButton(void* panel)`
- L444 `void* acc::menus::detail::SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox resolvers (see menus_internal.h).
- L449 `void* acc::menus::detail::SkillInfoBoxPanelMessageLabel(void* panel)`
- L454 `void* acc::menus::detail::SkillInfoBoxPanelOkButton(void* panel)`
- L462 `void* acc::menus::detail::ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect resolvers (see menus_internal.h).
- L467 `void* acc::menus::detail::ScriptSelectPanelAcceptButton(void* panel)`
- L472 `void* acc::menus::detail::ScriptSelectPanelBackButton(void* panel)`
- L479 `void* acc::menus::detail::WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup resolvers (see menus_internal.h).
- L485 `void* acc::menus::detail::WagerPopupPanelMoreButton(void* panel)`
- L491 `void* acc::menus::detail::WagerPopupPanelMaximumLabel(void* panel)`
- L498 `void* acc::menus::detail::PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart resolvers (see menus_internal.h).
- L504 `void* acc::menus::detail::PazaakStartPanelClearButton(void* panel)`
- L517 `void* acc::menus::detail::KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings resolvers (see menus_internal.h).
- L522 `void* acc::menus::detail::KeyMapPanelDefaultButton(void* panel)`
- L527 `void* acc::menus::detail::KeyMapPanelAcceptButton(void* panel)`
- L532 `void* acc::menus::detail::KeyMapPanelCancelButton(void* panel)`
- L537 `void* acc::menus::detail::KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L556 `void* acc::menus::detail::WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect resolvers (see menus_internal.h).
- L561 `void* acc::menus::detail::WorkbenchItemsPanelUpgradeButton(void* panel)`
- L566 `void* acc::menus::detail::WorkbenchItemsPanelBackButton(void* panel)`
- L576 `int acc::menus::detail::UpgradeSlotIndexFromButton(void* panel, void* control)`: Workbench-upgrade slot membership via the panel's embedded button run (one contiguous array in both games — see the kUpgradePanelSlotButtons note in engine_offsets_fields.h).
- L606 `bool acc::menus::detail::IsSaveLoadPanel(void* panel)`: Detect the CSWGuiSaveLoad panel (the "Spiel laden" / "Spiel speichern" dialog).
- L623 `const char* acc::menus::detail::ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: Read the user-visible text of a CExoString-style field on a control.
- L654 `bool acc::menus::detail::DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L676 `static bool DriveListBoxSelectionBody(void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`: The body, split out so the SEH wrapper above stays free of anything needing unwinding (MSVC C2712) — see the CLAUDE.md note on __try and objects.
- L742 `bool acc::menus::detail::DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L760 `static bool DriveListBoxSelectionEngineBody( void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`
- L828 `bool acc::menus::detail::QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control the caller resolved through an engine-member resolver.
- L849 `bool acc::menus::detail::IsClassSelectionIcon(void* panel, void* control)`: Positional detector for chargen class-icon buttons.
- L898 `void acc::menus::detail::ClassDescBaselineCapture(void* panel, const char* text)`
- L908 `bool acc::menus::detail::ClassDescIsEngineWritten(void* panel, const char* text)`
- L917 `const char* acc::menus::detail::ClassLabelCacheLookup(void* panel, void* icon)`
- L935 `void acc::menus::detail::ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose what the user hears for one class icon: the class name, followed by the blurb the engine renders beneath it, when there is one.
- L957 `void acc::menus::detail::ClassLabelCacheStore(void* panel, void* icon, const char* text)`
- L984 `void* acc::menus::detail::FindListBoxChild(void* panel)`: Find the first CSWGuiListBox child in a panel's controls.
