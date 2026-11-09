# menus_chain.cpp (1482 lines)

Source: `patches/Accessibility/menus_chain.cpp`

chain-navigation state and helpers.

here and what stays in menus.cpp. Function bodies are unchanged from the original menus.cpp inline definitions; only the namespacing / linkage changes.

## Declarations (in source order)

- L66 `int ComputeTabClickOffsetY(void* panel)`
- L84 `bool IsModalTextPanel(PanelKind k)`
- L107 `bool IsPanelLive(void* panel)`: Panels[]-membership check, shared by Validate{Tabbed,Chain}Panel and the DetectTabsCluster precondition.
- L124 `void* ReadPanelActiveControl(void* panel)`
- L130 `int FindChainEntry(void* control)`
- L145 `void SpeakLevelUpDoStepFirst()`: Speak guidance when the user activates an InGameLevelUp category that the engine has not currently enabled.
- L177 `bool DetectTabsCluster(void* panel, int& outStart, int& outCount)`
- L243 `void ResetTabbedState()`
- L249 `void RebindChainPreserveIndex(void* panel)`
- L261 `void InvalidateChain()`
- L276 `void ValidateTabbedPanel()`
- L286 `uintptr_t ReadVtable(void* obj)`: Read a panel/control's vtable word under SEH.
- L314 `bool ChainPanelIdentityHolds()`: Identity check for the panel the chain is bound to.
- L326 `void ValidateChainPanel()`
- L342 `bool IsTabButton(void* control)`
- L354 `void* FindAdjacentArrow(void* panel, void* focused, bool toRight)`
- L412 `bool LabelOpensWithWord(const char* text, const char* word)`: Does `text` OPEN WITH the whole word `word`?
- L423 `void* FindButtonByWords(void* panel, const char* const* words, int wordCount)`: Shared scan behind both finders below: first chain-navigable control whose caption opens with one of `words`.
- L443 `void* FindCloseButton(void* panel)`
- L453 `void* FindCancelButton(void* panel)`
- L470 `void AppendChainEntry(void* control, int sortCy = kSortByOwnCy)`: `sortCy` overrides only the reading-order key, never the real cy the cursor warp uses.
- L487 `void AppendChainTextOnly(void* control, void* panel)`
- L507 `void* ResolvePortraitChargenSkip(void* panel)`: Panel-aware chain filter: in CSWGuiPortraitCharGen we anchor the chain on the left_arrow alone (announces the current portrait via the PerKind path in menus_extract.cpp's section 9d) and consolidate the right_arrow ou...
- L526 `void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn, void*& storeToggleBtn, void*& storeAcceptBtn)`: Store-panel action buttons (Schliess.
- L546 `void* ResolveEquipPickerListBox(void* panel)`: InGameEquip picker listbox (LB_ITEMS, id=5): the equip panel keeps the picker's item listbox in panel.controls[] even when the picker isn't visually shown — the engine pre-populates it with the body- slot candidates a...
- L567 `void* ResolveWorkbenchUpgradeListBox(void* panel)`: WorkbenchUpgrade LB_ITEMS (id=0): the compatible-crystal list.
- L604 `bool IsDecorativeControl(void* panel, void* c, const char* closeCaption, bool haveCloseCaption)`: Per-kind decorative filter.
- L849 `void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb, void* workbenchUpgradeLb, bool modalText, int& listBlockTop)`: Listbox dispatch: * size > 1 — recurse one level into button children (sub-dialog settings list).
- L967 `bool AppendVirtualRow(void* labelControl, int sortCy, void* panel, RowProbe probe)`
- L981 `bool ProbeCreditsRow(void* panel, void* labelControl)`
- L987 `bool ProbeStatRow(void* panel, void* labelControl)`
- L993 `bool ProbeWagerRow(void* panel, void* labelControl)`
- L999 `bool ProbeEquipStatRow(void* panel, void* labelControl)`
- L1018 `bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData)`: Virtual credits row for Inventory + Store.
- L1037 `bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData)`: Per-kind virtual chain entries.
- L1045 `bool OnWagerAnchor(void* labelControl, int sortCy, void* userData)`: Virtual wager row for the Pazaak wager popup.
- L1056 `bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData)`: Virtual stat rows for the Equip panel (Vitality, Defense, Attack, Damage).
- L1067 `bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy, void* /*userData*/)`: Virtual "Mod settings" entry for InGameOptions + MainMenuOptions.
- L1090 `void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n)`: Per-kind virtual chain entries for StatusSummary — the engine's quest- progress / journal-entry info popup.
- L1109 `bool ChainEntryPrecedes(const ChainEntry& a, const ChainEntry& b)`: Reading-order comparator: y first, then x — but the x tiebreak applies only when BOTH entries are geometrically ordered (see ChainEntry::geometricOrder).
- L1145 `void DemoteFilterStripAboveList(void* panel, int listBlockTop)`: House rule: on a list-driven screen the list is read first and its filter / toggle strip after it.
- L1172 `void SortChainBySortCy()`: Insertion sort in reading order (ChainEntryPrecedes).
- L1187 `void SquashCycleFlankers(void* panel)`: Squash cycle-arrow flankers from the chain.
- L1245 `void ComputeEquipSlotClickOffset(void* panel)`: Row pitch between InGameEquip slot buttons, used to compensate the simulated click position.
- L1280 `void ComputeClassIconClickOffset(void* panel)`: Column pitch between chargen class-selection icons, same purpose as ComputeEquipSlotClickOffset above.
- L1306 `void LogChainContents(void* panel, void* active)`: Per-rebind chain dump.
- L1352 `void RebindChain(void* panel)`
