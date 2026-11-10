// Internal to the menus_chain module (menus_chain*.cpp); no other file includes
// it. The public interface is menus_chain.h. Here: what the files share.

#pragma once

#include "menus_chain.h"

#include <windows.h>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_manager.h"

#include "engine_offsets.h"
#include "menus_internal.h"

using namespace acc::engine;  // IdentifyPanel, PanelKind, kInput*, etc.

using acc::menus::detail::IsChainNavigable;
using acc::menus::detail::IsClassSelectionIcon;
using acc::menus::detail::GetControlCenter;
using acc::menus::detail::ScaleGuiThresholdPx;

namespace acc::menus::chain {

// ============================================================================
// State definitions (extern decls live in menus_chain.h).
// ============================================================================
extern ChainEntry g_chain[kMaxChainEntries];
extern void*      g_chainPanel;
extern uintptr_t  g_chainVtable;
extern int        g_chainIndex;
extern int        g_chainCount;

extern void* g_tabbedPanel;
extern int   g_tabsStart;
extern int   g_tabsCount;

extern int g_equipSlotClickOffsetY;
extern int g_classIconClickOffsetX;

// Helpers shared across the files, by the file that defines them.

// menus_chain.cpp
bool IsPanelLive(void* panel);

// menus_chain_filter.cpp
void* ResolvePortraitChargenSkip(void* panel);
void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn, void*& storeToggleBtn, void*& storeAcceptBtn);
void* ResolveEquipPickerListBox(void* panel);
void* ResolveWorkbenchUpgradeListBox(void* panel);
bool IsDecorativeControl(void* panel, void* c, const char* closeCaption, bool haveCloseCaption);

// Sentinel for AppendChainEntry's sortCy parameter: sort this entry by its
// own cy, the normal case for a panel-direct control.
constexpr int kSortByOwnCy = INT_MIN;

// menus_chain_build.cpp
void AppendChainEntry(void* control, int sortCy = kSortByOwnCy);
void AppendListBoxChildren(void* panel, void* c, void* equipPickerLb, void* workbenchUpgradeLb, bool modalText, int& listBlockTop);
bool OnCreditsAnchor(void* labelControl, int sortCy, void* userData);
bool OnStatRowAnchor(void* labelControl, int sortCy, void* userData);
bool OnWagerAnchor(void* labelControl, int sortCy, void* userData);
bool OnEquipStatAnchor(void* labelControl, int sortCy, void* userData);
bool OnModSettingsAnchor(void* sentinel, int sortCx, int sortCy, void* );
void AppendStatusSummaryRows(void* panel, CExoArrayList* list, int n);
void DemoteFilterStripAboveList(void* panel, int listBlockTop);
void SortChainBySortCy();
void SquashCycleFlankers(void* panel);
void ComputeEquipSlotClickOffset(void* panel);
void ComputeClassIconClickOffset(void* panel);

}  // namespace acc::menus::chain
