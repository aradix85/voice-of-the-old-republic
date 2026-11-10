# menus_internal.cpp (983 lines)

Source: `patches/Accessibility/menus_internal.cpp`

Implementations of the acc::menus::detail:: seam declared in menus_internal.h.

These helpers were defined in menus.cpp until the Phase-1 structure pass (refactoring candidate 1). menus_internal.h has always declared them as the "internal-but-exposed-across-TUs" contract shared by menus.cpp, menus_extract.cpp, menus_listbox.cpp and menus_keymap.cpp; this file is the matching .cpp every other seam header in this directory already has (menus_chain, menus_pending, menus_listbox, menus_editbox, menus_monitors).

Nothing here changed behaviourally in the move: the definitions are verbatim, including the saveload.gui control IDs and the chargen class-label cache, both of which are used only by functions in this file.

## Declarations (in source order)

- L52 `inline int SaveLoadLbGamesId()`: CSWGuiSaveLoad control ids from saveload.gui / K2 saveload_p.gui (verified against chain logs: patch-20260505-160124.log lines 45-65; K2 witnessed live in patch-20260813-212757 — K2 re-authored the screen and every id...
- L53 `inline int SaveLoadBtnBackId()`
- L54 `inline int SaveLoadBtnSaveLoadId()`
- L61 `int ScaleGuiThresholdPx(int guiPx)`: See menus_internal.h for why this exists and when it is a no-op.
- L85 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: Center pixel of a control's hit area.
- L120 `bool IsChainNavigable(void* control)`: Screen-absolute center of a CSWGuiListBox row.
- L136 `void* FindControlById(void* panel, int id)`: Locate a child control on `panel` by its +0x50 ID field.
- L163 `int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff, size_t stride)`
- L178 `int EquipSlotIndexFromButton(void* panel, void* control)`
- L184 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L201 `void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId, const char* tag)`
- L217 `void* EquipPanelItemsListBox(void* panel)`
- L222 `void* EquipPanelEquipButton(void* panel)`
- L227 `void* EquipPanelBackButton(void* panel)`
- L232 `void* UpgradePanelItemsListBox(void* panel)`
- L238 `void* UpgradePanelAssembleButton(void* panel)`
- L246 `void* UpgradePanelBackButton(void* panel)`
- L256 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad — same engine-truth resolvers (members mined from both games' ctors, see kSaveLoadPanel*Offset).
- L261 `void* SaveLoadPanelActionButton(void* panel)`
- L267 `void* SaveLoadPanelBackButton(void* panel)`
- L281 `void* CraftMemberWithTripwire(void* panel, size_t componentOff, size_t chemicalOff, int guiId, const char* tag)`: The two crafting classes lay the same controls out at different offsets; pick per panel kind, or return nullptr when `panel` is neither.
- L295 `void* UpgradeSelPanelListBox(void* panel)`
- L301 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L307 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L312 `void* CraftPanelShopListBox(void* panel)`
- L318 `void* CraftPanelInvListBox(void* panel)`
- L324 `void* CraftPanelAcceptButton(void* panel)`
- L330 `void* CraftPanelExamineButton(void* panel)`
- L339 `void* CraftPanelTitleLabel(void* panel)`
- L353 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp resolvers (see menus_internal.h).
- L359 `void* PowersPanelDescListBox(void* panel)`
- L365 `void* PowersPanelPowerLabel(void* panel)`
- L371 `void* PowersPanelSubTitleLabel(void* panel)`
- L377 `void* PowersPanelRecommendedButton(void* panel)`
- L383 `void* PowersPanelAcceptButton(void* panel)`
- L389 `void* PowersPanelBackButton(void* panel)`
- L398 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer resolvers (see menus_internal.h).
- L403 `void* ContainerPanelOkButton(void* panel)`
- L408 `void* ContainerPanelCancelButton(void* panel)`
- L413 `void* ContainerPanelGiveItemsButton(void* panel)`
- L426 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen resolvers (see menus_internal.h).
- L432 `void* FeatsPanelBackButton(void* panel)`
- L438 `void* FeatsPanelRecommendedButton(void* panel)`
- L446 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox resolvers (see menus_internal.h).
- L451 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L456 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L464 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect resolvers (see menus_internal.h).
- L469 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L474 `void* ScriptSelectPanelBackButton(void* panel)`
- L481 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup resolvers (see menus_internal.h).
- L487 `void* WagerPopupPanelMoreButton(void* panel)`
- L493 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L500 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart resolvers (see menus_internal.h).
- L506 `void* PazaakStartPanelClearButton(void* panel)`
- L519 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings resolvers (see menus_internal.h).
- L524 `void* KeyMapPanelDefaultButton(void* panel)`
- L529 `void* KeyMapPanelAcceptButton(void* panel)`
- L534 `void* KeyMapPanelCancelButton(void* panel)`
- L539 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L558 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect resolvers (see menus_internal.h).
- L563 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L568 `void* WorkbenchItemsPanelBackButton(void* panel)`
- L578 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Workbench-upgrade slot membership via the panel's embedded button run (one contiguous array in both games — see the kUpgradePanelSlotButtons note in engine_offsets_fields.h).
- L608 `bool IsSaveLoadPanel(void* panel)`: Detect the CSWGuiSaveLoad panel (the "Load Game" / "Save Game" dialog).
- L625 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: Read the user-visible text of a CExoString-style field on a control.
- L656 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L678 `static bool DriveListBoxSelectionBody(void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`: The body, split out so the SEH wrapper above stays free of anything needing unwinding (MSVC C2712) — see the CLAUDE.md note on __try and objects.
- L744 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`
- L762 `static bool DriveListBoxSelectionEngineBody( void* listbox, acc::menus::detail::ListBoxNavOp op, short minSel, acc::menus::detail::ListBoxNavResult& out)`
- L830 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control the caller resolved through an engine-member resolver.
- L851 `bool IsClassSelectionIcon(void* panel, void* control)`: Positional detector for chargen class-icon buttons.
- L900 `void ClassDescBaselineCapture(void* panel, const char* text)`
- L910 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L919 `const char* ClassLabelCacheLookup(void* panel, void* icon)`
- L937 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose what the user hears for one class icon: the class name, followed by the blurb the engine renders beneath it, when there is one.
- L959 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
