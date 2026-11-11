// Repairing the inventory's lazily filled item list: when the list is still
// empty with the screen in front, run the engine's repopulate once.

#include "menus_monitors_internal.h"

#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus_chain.h"
#include "menus_inventory.h"

namespace acc::menus::monitors {

namespace {

// The inventory's item listbox is filled LAZILY: the engine rebuilds it from
// CSWGuiInGameInventory::Draw a frame after the screen appears, and after
// anything changes what the party carries the visit can start with the list
// still empty. A sighted player never notices that frame. Our chain does — it
// binds on open, finds no rows, and the screen then offers nothing but its
// filter strip for the rest of the visit, because nothing invalidates the
// chain afterwards.
//
// On KOTOR 2 that left two items in the pack unreachable while the filters
// cycled, until a filter was activated — the filter path already forces a
// repopulate.
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
    return acc::engine::ReadListBoxRowCount(
        acc::off::Ptr(panel, kInventoryItemListBoxOffset));
}

}  // namespace

void MonitorInventoryListPopulation() {
    void* mgr = GetGuiManager();
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
