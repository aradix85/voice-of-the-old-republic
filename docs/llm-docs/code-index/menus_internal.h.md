# menus_internal.h (478 lines)

Source: `patches/Accessibility/menus_internal.h`

internal seam between menus.cpp and menus_extract.cpp.

This is NOT public API; it's a private contract between the two TUs that were one TU before Step 2B of the refactor. Module names declared here live in `acc::menus::detail::` to signal "internal-but-exposed-across- TUs."

What the seam covers, and why each thing is here:

* g_currentPanel — the focused-panel pointer maintained by OnSetActiveControl (in menus.cpp) and read by ExtractAnnounceableText (in menus_extract.cpp) for the slider sibling-label / cycle-flanker gates and the per-kind owner-panel fallback.

* IsChainNavigable — predicate for "can the user focus this control with arrow keys?" Used by chain code in menus.cpp (RebindChain, AppendChainEntry, FindCloseButton/CancelButton, FindAdjacentArrow) AND by the sibling-label fallback in extract.

* IsClassSelectionIcon — positional detector for chargen class-icon buttons. Used by extract's perkind-classsel branch AND by menus.cpp chain code (cursor x-offset compensation, RebindChain pitch capture, OnSetActiveControl prefill, AnnounceControl cache-aware skip).

* ClassLabelCacheLookup / Store — lazily-populated (icon → class_text) cache for the chargen class-selection panel. The cache state lives in menus.cpp (file-static); the extract path reads (lookup), the OnSetActiveControl prefill writes (store). Lookup-then-store happens inside extract step 9c too, so both directions cross the seam.

* kEquipBtn* / kEquipLb* — control IDs from equip.gui. Used by extract's perkind-InGameEquip branch (slot-button label resolution) AND by the OnHandleInputEvent equip-slot detector / equip-picker handler in menus.cpp.

The in-game-menu-icon name table stays private to its owning TU because only one side uses it. `kSaveLoad*` moved to menus_internal.cpp with IsSaveLoadPanel, its only consumer.

The declarations in this header are implemented in menus_internal.cpp (they lived in menus.cpp until the Phase-1 structure pass split that file into menus.cpp / menus_internal.cpp / menus_focus.cpp / menus_dispatch.cpp).

## Declarations (in source order)

- L66 `bool IsChainNavigable(void* control)`
- L67 `bool IsClassSelectionIcon(void* panel, void* control)`
- L68 `const char* ClassLabelCacheLookup(void* panel, void* icon)`
- L69 `void ClassLabelCacheStore(void* panel, void* icon, const char* text)`
- L74 `void ComposeClassAnnounce(void* panel, const char* name, char* outBuf, size_t bufSize)`: Compose "class name" + the engine's class blurb into one announce string.
- L82 `void ClassDescBaselineCapture(void* panel, const char* text)`: Class-description placeholder discrimination.
- L83 `bool ClassDescIsEngineWritten(void* panel, const char* text)`
- L107 `int ScaleGuiThresholdPx(int guiPx)`: Convert a distance measured in .gui authoring units into the units GetControlCenter below actually returns on the running game.
- L112 `bool GetControlCenter(void* control, int& outCx, int& outCy)`: Screen-center of a control's extent, false on degenerate extents.
- L118 `void* FindControlById(void* panel, int id)`: Locate a child control on `panel` by its +0x50 ID field.
- L124 `bool IsSaveLoadPanel(void* panel)`: Detect the CSWGuiSaveLoad panel by its vtable (kVtableCSWGuiSaveLoad) — the ctor-written identity of this dynamically allocated panel, immune to .gui renumbering.
- L129 `const char* ReadSaveLoadEntryString(void* entry, size_t fieldOffset)`: Read a CExoString-style field on a control by byte offset.
- L136 `struct ListBoxNavResult`: Result of a single arrow-key step on a listbox cursor.
- L149 `enum class ListBoxNavOp`: Direction of cursor motion driven by DriveListBoxSelection.
- L158 `bool DriveListBoxSelection(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Drive a CSWGuiListBox cursor in response to Nav-Up / Nav-Down / Home / End.
- L171 `bool DriveListBoxSelectionEngine(void* listbox, ListBoxNavOp op, short minSel, ListBoxNavResult& out)`: Same contract as DriveListBoxSelection, but drives the engine's own CSWGuiListBox::SetSelectedControl instead of writing selection_index raw.
- L184 `bool ParkCursorToCorner(const char* tag)`: Warp the OS cursor to the empty top-left corner of the screen so the engine's per-frame hover-select (CSWGuiListBox::HandleMouseMove -> HitCheckMouseLocal -> SetSelectedControl) finds no row under it and stops overwri...
- L191 `bool QueueControlActivate(void* target, const char* logPrefix)`: Queue activation of a control resolved through an engine-member resolver (SaveLoadPanel* / ContainerPanel* / ...).
- L213 `void* CurrentPanelIfLive()`: Liveness-filtered view of g_currentPanel — returns it only while it is still in the manager's panels[], nullptr once the engine has freed it.
- L299 `int EquipSlotIndexFromButton(void* panel, void* control)`: Equip-panel slot identity from the engine's embedded control arrays (ctor-mined bases/strides — kEquipPanelSlotButtons/LabelsOffset).
- L300 `int EquipSlotIndexFromControl(void* panel, void* control)`
- L303 `inline int EquipSlotCount()`: Slots in the engine's arrays for the running game (9 on K1, 11 on K2).
- L312 `void* EquipPanelItemsListBox(void* panel)`: Engine-truth resolvers for the equip panel's picker members — the EMBEDDED items_listbox ("LB_ITEMS"), equip button ("BTN_EQUIP") and back button ("BTN_BACK") the constructors bind by tag.
- L313 `void* EquipPanelEquipButton(void* panel)`
- L314 `void* EquipPanelBackButton(void* panel)`
- L319 `void* UpgradePanelItemsListBox(void* panel)`: Workbench upgrade panel (CSWGuiUpgrade) — same engine-truth resolvers.
- L320 `void* UpgradePanelAssembleButton(void* panel)`
- L321 `void* UpgradePanelBackButton(void* panel)`
- L329 `int UpgradeSlotIndexFromButton(void* panel, void* control)`: Slot-button membership on the upgrade panel via the embedded button run (K1: 7 from +0x64, K2: 9 from +0x7a8).
- L330 `inline int UpgradeSlotButtonCount()`
- L336 `void* SaveLoadPanelGamesListBox(void* panel)`: CSWGuiSaveLoad — same engine-truth resolvers (ctor-bound embedded members, kSaveLoadPanel*Offset; ids only feed the GuiIdMismatch tripwire).
- L337 `void* SaveLoadPanelActionButton(void* panel)`
- L338 `void* SaveLoadPanelBackButton(void* panel)`
- L352 `void* UpgradeSelPanelListBox(void* panel)`: K2 workbench/crafting engine-truth resolvers (ctor-bound embedded members, kUpgradeSelPanelkCraftComponentkCraftChemical* in engine_offsets_fields.h; the historical .gui ids only feed the GuiIdMismatch tripwire).
- L353 `void* UpgradeSelPanelUpgradeButton(void* panel)`
- L354 `void* UpgradeSelPanelTitleLabel(void* panel)`
- L355 `void* CraftPanelShopListBox(void* panel)`
- L356 `void* CraftPanelInvListBox(void* panel)`
- L357 `void* CraftPanelAcceptButton(void* panel)`
- L358 `void* CraftPanelExamineButton(void* panel)`
- L359 `void* CraftPanelTitleLabel(void* panel)`
- L368 `void* PowersPanelPowersListBox(void* panel)`: CSWGuiPowersLevelUp (pwrlvlup / pwrlvlup_p) engine-truth resolvers — ctor-bound embedded members, kPowersPanel*Offset in engine_offsets_fields.h.
- L369 `void* PowersPanelDescListBox(void* panel)`
- L370 `void* PowersPanelPowerLabel(void* panel)`
- L371 `void* PowersPanelSubTitleLabel(void* panel)`
- L372 `void* PowersPanelRecommendedButton(void* panel)`
- L373 `void* PowersPanelAcceptButton(void* panel)`
- L374 `void* PowersPanelBackButton(void* panel)`
- L383 `void* ContainerPanelItemsListBox(void* panel)`: CSWGuiContainer (container / container_p) engine-truth resolvers — ctor-bound embedded members, kContainerPanel*Offset in engine_offsets_fields.h.
- L384 `void* ContainerPanelOkButton(void* panel)`
- L385 `void* ContainerPanelCancelButton(void* panel)`
- L386 `void* ContainerPanelGiveItemsButton(void* panel)`
- L397 `void* FeatsPanelAcceptButton(void* panel)`: CSWGuiFeatsCharGen (ftchrgen / ftchrgen_p) engine-truth resolvers — ctor-bound embedded members, kFeatsCharGen*ButtonOffset in engine_offsets_fields.h.
- L398 `void* FeatsPanelBackButton(void* panel)`
- L399 `void* FeatsPanelRecommendedButton(void* panel)`
- L406 `void* SkillInfoBoxPanelSkillsListBox(void* panel)`: CSWGuiSkillInfoBox (skillinfo / skillinfo_p) engine-truth resolvers — the granted-feats / level-up info popup.
- L407 `void* SkillInfoBoxPanelMessageLabel(void* panel)`
- L408 `void* SkillInfoBoxPanelOkButton(void* panel)`
- L415 `void* ScriptSelectPanelAiStateListBox(void* panel)`: CSWGuiScriptSelect (ScriptSelect.gui) engine-truth resolvers — the character sheet's combat-behaviour picker, KOTOR 1 only.
- L416 `void* ScriptSelectPanelAcceptButton(void* panel)`
- L417 `void* ScriptSelectPanelBackButton(void* panel)`
- L425 `void* WagerPopupPanelLessButton(void* panel)`: CSWGuiWagerPopup (pazaakwager / pazaakwager_p) engine-truth resolvers — the pazaak bet popup.
- L426 `void* WagerPopupPanelMoreButton(void* panel)`
- L427 `void* WagerPopupPanelMaximumLabel(void* panel)`
- L434 `void* PazaakStartPanelPlayButton(void* panel)`: CSWGuiPazaakStart (pazaaksetup / pazaaksetup_p) engine-truth resolvers — the side-deck builder.
- L435 `void* PazaakStartPanelClearButton(void* panel)`
- L444 `void* KeyMapPanelEventListBox(void* panel)`: CSWGuiInGameOptKeyMappings (OPTKeyMapping / OPTKeyMapping_p) engine-truth resolvers — the Keymapping screen.
- L445 `void* KeyMapPanelDefaultButton(void* panel)`
- L446 `void* KeyMapPanelAcceptButton(void* panel)`
- L447 `void* KeyMapPanelCancelButton(void* panel)`
- L448 `void* KeyMapPanelFilterButton(void* panel, int filterIndex)`
- L454 `void* WorkbenchItemsPanelItemsListBox(void* panel)`: CSWGuiUpgradeItemSelect (upgradeitems / upgradeitems_p) engine-truth resolvers — the workbench per-category item picker.
- L455 `void* WorkbenchItemsPanelUpgradeButton(void* panel)`
- L456 `void* WorkbenchItemsPanelBackButton(void* panel)`

Also 24 constants or variables at namespace scope.
