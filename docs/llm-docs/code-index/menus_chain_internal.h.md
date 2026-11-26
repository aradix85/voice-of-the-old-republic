# menus_chain_internal.h (65 lines)

Source: `patches/Accessibility/menus_chain_internal.h`

Internal to the menus_chain module (menus_chain*.cpp); no other file includes it. The public interface is menus_chain.h. Here: what the files share.

## Declarations (in source order)

- L35 `bool IsPanelLive(void* panel)`: menus_chain.cpp
- L38 `void* ResolvePortraitChargenSkip(void* panel)`: menus_chain_filter.cpp
- L39 `void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn, void*& storeToggleBtn, void*& storeAcceptBtn)`
- L40 `void* ResolveEquipPickerListBox(void* panel)`
- L41 `void* ResolveWorkbenchUpgradeListBox(void* panel)`
- L42 `bool IsDecorativeControl(void* panel, void* c, const char* closeCaption, bool haveCloseCaption)`
- L49 `void AppendChainEntry(void* control, int sortCy = kSortByOwnCy)`: menus_chain_build.cpp
- L50 `void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb, void* workbenchUpgradeLb, bool modalText, int& listBlockTop)`
- L51 `bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData)`
- L52 `bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData)`
- L53 `bool OnWagerAnchor(void* labelControl, int sortCy, void* userData)`
- L54 `bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData)`
- L55 `bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy, void* )`
- L56 `void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n)`
- L57 `void DropCoveredButtons()`
- L58 `void DemoteFilterStripAboveList(void* panel, int listBlockTop)`
- L59 `void SortChainBySortCy()`
- L60 `void SquashCycleFlankers(void* panel)`
- L61 `void ComputeEquipSlotClickOffset(void* panel)`
- L62 `void ComputeClassIconClickOffset(void* panel)`

Also 7 constants or variables at namespace scope.
