// Tabbed panels: finding a panel's row of tab buttons and where a click on a tab lands.

#include "menus_chain_internal.h"

#include "engine_offsets.h"
#include "engine_panels.h"
#include "log.h"
#include "menus_store.h"

namespace acc::menus::chain {

int ComputeTabClickOffsetY(void* panel) {
    if (!panel || g_tabbedPanel != panel || g_tabsCount < 2) return 0;
    int firstTabIdx = -1;
    for (int i = 0; i < g_chainCount; ++i) {
        if (!IsTabButton(g_chain[i].control)) continue;
        if (firstTabIdx < 0) {
            firstTabIdx = i;
        } else {
            int spacing = g_chain[i].cy - g_chain[firstTabIdx].cy;
            return spacing > 0 ? spacing : 0;
        }
    }
    return 0;
}

// ============================================================================
// Tab-cluster detection.
// ============================================================================

bool DetectTabsCluster(void* panel, int& outStart, int& outCount) {
    outStart = -1;
    outCount = 0;
    if (!panel) return false;

    // Guard against a stale g_currentPanel that the engine already freed.
    // See IsPanelLive comment above for the save-popup teardown crash.
    if (!IsPanelLive(panel)) {
        acclog::Write("DetectTabsCluster", "%p not in panels[]; skipping", panel);
        return false;
    }

    // Store is structurally tab-cluster-shaped (listbox @ controls[0],
    // three navigable buttons clumped further down) but the three buttons
    // are Sell / Close / Buy — distinct action buttons,
    // not page tabs. Flagging them as tabs makes the chain Enter handler
    // route through click-sim (the tab path) instead of FireActivate,
    // which lands the click on whatever overlay sits at the button's
    // y-coordinate — in the wild that misroutes Close into the engine
    // quit-confirm popup.
    if (acc::menus::store::IsStorePanel(panel)) return false;

    // The workbench upgrade panel (upgrade.gui) is the same shape: LB_ITEMS
    // at controls[0] followed by the seven slot buttons (ids 12-18) as a
    // contiguous navigable run. Those are slot pickers, not page tabs.
    // Latching them as tabs routes their Enter through the tab click-sim path
    // (which opens nothing) instead of the chain handler's
    // OnEnterSlot+OnSlotSelected arm. The detection fired the first time only
    // after the user opened a slot, so re-entering any slot after a picker Esc
    // did nothing until the workbench was closed and reopened
    // (patch-20260616-141620.log).
    if (acc::engine::IdentifyPanel(panel) ==
            acc::engine::PanelKind::WorkbenchUpgrade) {
        return false;
    }

    auto* panelList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!panelList->data || panelList->size < 2) return false;

    void* lb = panelList->data[0];
    if (!lb) return false;
    void** vt = *reinterpret_cast<void***>(lb);
    if (reinterpret_cast<uintptr_t>(vt) != kVtableListBox) return false;

    auto* lbList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(lb) + kListBoxControlsOffset);
    if (!lbList->data || lbList->size <= 0) return false;

    int n = panelList->size > 256 ? 256 : panelList->size;
    int start = -1, end = -1;
    for (int i = 1; i < n; ++i) {
        void* c = panelList->data[i];
        if (c && IsChainNavigable(c)) {
            if (start < 0) start = i;
            end = i;
        } else if (start >= 0) {
            break;
        }
    }
    if (start < 0 || (end - start + 1) < 2) return false;
    outStart = start;
    outCount = end - start + 1;
    return true;
}

// ============================================================================
// Chain builders + cycle-arrow + button finders.
// ============================================================================

bool IsTabButton(void* control) {
    if (!control || !g_tabbedPanel || g_tabsCount < 2) return false;
    auto* tlist = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(g_tabbedPanel) + kPanelControlsOffset);
    if (!tlist || !tlist->data) return false;
    for (int i = g_tabsStart;
         i < g_tabsStart + g_tabsCount && i < tlist->size; ++i) {
        if (tlist->data[i] == control) return true;
    }
    return false;
}

}  // namespace acc::menus::chain
