# menus_chain_build.cpp (518 lines)

Source: `patches/Accessibility/menus_chain_build.cpp`

Building the chain's entries: controls, list-box rows, the virtual rows for value labels (credits, stats, wager, equipment stats, mod settings), the reading order, and the click offsets for slots and class icons.

## Declarations (in source order)

- L44 `void AppendChainTextOnly(void* control, void* panel)`
- L57 `bool AppendVirtualRow(void* labelControl, int sortCy, void* panel, RowProbe probe)`
- L71 `bool ProbeCreditsRow(void* panel, void* labelControl)`
- L77 `bool ProbeStatRow(void* panel, void* labelControl)`
- L83 `bool ProbeWagerRow(void* panel, void* labelControl)`
- L89 `bool ProbeEquipStatRow(void* panel, void* labelControl)`
- L100 `bool ChainEntryPrecedes(const ChainEntry& a, const ChainEntry& b)`: Reading-order comparator: y first, then x — but the x tiebreak applies only when BOTH entries are geometrically ordered (see ChainEntry::geometricOrder).
- L111 `void AppendChainEntry(void* control, int sortCy)`: `sortCy` overrides only the reading-order key, never the real cy the cursor warp uses.
- L134 `void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb, void* workbenchUpgradeLb, bool modalText, int& listBlockTop)`: Listbox dispatch: * size > 1 — recurse one level into button children (sub-dialog settings list).
- L249 `bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData)`: Virtual credits row for Inventory + Store.
- L268 `bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData)`: Per-kind virtual chain entries.
- L276 `bool OnWagerAnchor(void* labelControl, int sortCy, void* userData)`: Virtual wager row for the Pazaak wager popup.
- L287 `bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData)`: Virtual stat rows for the Equip panel (Vitality, Defense, Attack, Damage).
- L298 `bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy, void* /*userData*/)`: Virtual "Mod settings" entry for InGameOptions + MainMenuOptions.
- L321 `void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n)`: Per-kind virtual chain entries for StatusSummary — the engine's quest- progress / journal-entry info popup.
- L359 `void DemoteFilterStripAboveList(void* panel, int listBlockTop)`: House rule: on a list-driven screen the list is read first and its filter / toggle strip after it.
- L386 `void SortChainBySortCy()`: Insertion sort in reading order (ChainEntryPrecedes).
- L401 `void SquashCycleFlankers(void* panel)`: Squash cycle-arrow flankers from the chain.
- L459 `void ComputeEquipSlotClickOffset(void* panel)`: Row pitch between InGameEquip slot buttons, used to compensate the simulated click position.
- L494 `void ComputeClassIconClickOffset(void* panel)`: Column pitch between chargen class-selection icons, same purpose as ComputeEquipSlotClickOffset above.
