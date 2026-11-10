// Repairing the inventory's lazily filled item list: when the list is still empty with the screen in front, run the engine's repopulate once.

#include "menus_monitors_internal.h"

#include "engine_offsets.h"
#include "menus_chain.h"
#include "menus_inventory.h"  // ForceRepopulate - lazy item-list repair

namespace acc::menus::monitors {

namespace {

// ============================================================================
// Inventory item-list population repair.
// ============================================================================
// The inventory's item listbox is filled LAZILY: the engine rebuilds it from
// CSWGuiInGameInventory::Draw a frame after the screen appears, and after
// anything changes what the party carries the visit can start with the list
// still empty. A sighted player never notices that frame. Our chain does — it
// binds on open, finds no rows, and the screen then offers nothing but its
// filter strip for the rest of the visit, because nothing invalidates the
// chain afterwards.
//
// The KOTOR 2 tester who reported "inventory broken" sat in exactly that
// state: two items in the pack, fourteen seconds of cycling All / Datapads /
// Weapons with no way to reach either, and the items appeared only when a
// filter was activated — because the filter path already forces a
// repopulate (userlogs/daninventoryequipmentbroken077: chain count=7 at
// 13:50:03, count=9 the moment "All" was pressed at 13:50:17).
//
// So run that same repopulate the moment the screen is in front of the user
// with an empty list, instead of waiting for a keypress to do it by accident.
//
// Latched per panel and released as soon as rows exist, which bounds the
// cost to one call per empty streak — an inventory that is genuinely empty
// pays exactly one no-op PopulateItemListBox per visit, not one per tick.
// Runs from the tick, never from the input hook: PopulateItemListBox frees
// and rebuilds row controls, and this is the same context the pending-op
// drain already dispatches that kind of deep engine call from.
void* g_inventoryEmptyLatch = nullptr;

// Rows currently in the inventory's item listbox. -1 when the read faults —
// the panel can be mid-teardown — which the caller reads as "unknown, look
// again next tick" rather than as "empty".
int InventoryItemRowCount(void* panel) {
    __try {
        auto* rows = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(panel) +
            kInventoryItemListBoxOffset + kListBoxControlsOffset);
        if (!rows->data) return 0;
        return rows->size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

}  // namespace

void MonitorInventoryListPopulation() {
    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    if (!mgr) { g_inventoryEmptyLatch = nullptr; return; }
    void* fg = GetForegroundPanel(mgr);
    if (!fg || IdentifyPanel(fg) != PanelKind::InGameInventory) {
        g_inventoryEmptyLatch = nullptr;
        return;
    }

    int rowCount = InventoryItemRowCount(fg);
    if (rowCount < 0) return;                  // unreadable this tick
    if (rowCount > 0) {
        g_inventoryEmptyLatch = nullptr;       // populated - re-arm
        return;
    }
    if (g_inventoryEmptyLatch == fg) return;   // already tried for this streak
    g_inventoryEmptyLatch = fg;

    acclog::Write("Menus.Inventory",
                  "item list empty on foreground panel=%p — forcing populate",
                  fg);
    acc::menus::inventory::ForceRepopulate(fg);
    // Rebind only when the chain is the thing that would otherwise go stale.
    // If it is not bound to this panel yet — the common case, since the chain
    // binds on the first nav key — there is nothing to repair and the user's
    // first arrow press builds it against the now-populated list.
    if (acc::menus::chain::g_chainPanel == fg) {
        acc::menus::chain::RebindChainPreserveIndex(fg);
    }
}

}  // namespace acc::menus::monitors
