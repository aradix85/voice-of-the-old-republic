# menus_chain_internal.h (76 lines)

Source: `patches/Accessibility/menus_chain_internal.h`

Internal to the menus_chain module (menus_chain*.cpp); no other file includes it. The public interface is menus_chain.h. Here: what the files share.

## Declarations (in source order)

- L47 `bool IsPanelLive(void* panel)`: menus_chain.cpp
- L50 `void* ResolvePortraitChargenSkip(void* panel)`: menus_chain_filter.cpp
- L51 `void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn, void*& storeToggleBtn, void*& storeAcceptBtn)`
- L52 `void* ResolveEquipPickerListBox(void* panel)`
- L53 `void* ResolveWorkbenchUpgradeListBox(void* panel)`
- L54 `bool IsDecorativeControl(void* panel, void* c, const char* closeCaption, bool haveCloseCaption)`
- L61 `void AppendChainEntry(void* control, int sortCy = kSortByOwnCy)`: menus_chain_build.cpp
- L62 `void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb, void* workbenchUpgradeLb, bool modalText, int& listBlockTop)`
- L63 `bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData)`
- L64 `bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData)`
- L65 `bool OnWagerAnchor(void* labelControl, int sortCy, void* userData)`
- L66 `bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData)`
- L67 `bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy, void* )`
- L68 `void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n)`
- L69 `void DemoteFilterStripAboveList(void* panel, int listBlockTop)`
- L70 `void SortChainBySortCy()`
- L71 `void SquashCycleFlankers(void* panel)`
- L72 `void ComputeEquipSlotClickOffset(void* panel)`
- L73 `void ComputeClassIconClickOffset(void* panel)`

Also 16 constants or variables at namespace scope.
