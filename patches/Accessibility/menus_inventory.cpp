#include "menus_inventory.h"

#include "engine_offsets.h"
#include "engine_addr.h"
#include "engine_reads.h"
#include "log.h"

#include <cstddef>
#include <cstdint>

#include <windows.h>

namespace acc::menus::inventory {

namespace {

typedef void (__thiscall* PFN_PanelThiscall)(void* panel);

}  // namespace

bool IsUseItemButton(void* panel, void* control) {
    return acc::off::IsAt(panel, kInventoryUseItemButtonOffset, control);
}

bool IsFilterButton(void* panel, void* control) {
    // KOTOR 2 replaced K1's single filter-CYCLE button with seven direct
    // filter buttons laid out one button-stride apart (BTN_ALL..BTN_QUESTS).
    // Any of them changes the filtered list, so all get the repopulate-
    // then-invalidate treatment.
    if (acc::game::IsKotor2()) {
        if (!panel || !control) return false;
        auto base = reinterpret_cast<uintptr_t>(panel);
        auto c    = reinterpret_cast<uintptr_t>(control);
        if (c < base + kInventoryFilterFirstButtonOffset ||
            c > base + kInventoryFilterLastButtonOffset) return false;
        return ((c - base) - kInventoryFilterFirstButtonOffset) %
                   kInventoryFilterButtonStride == 0;
    }
    return acc::off::IsAt(panel, kInventoryFilterButtonOffset, control);
}

void SyncListBoxSelection(void* panel, void* row) {
    if (!panel || !row) return;
    if (!acc::addr::Ok(kAddrCSWGuiListBoxSetSelectedControl)) {
        acclog::Write("Menus.Inventory",
                      "SyncListBoxSelection skipped (unresolved on build %s)",
                      acc::game::TitleName());
        return;
    }
    void* lb = acc::off::Ptr(panel, kInventoryItemListBoxOffset);
    const int idx = acc::engine::FindListBoxRow(lb, row);
    if (idx < 0) {
        // The row is not in item_listbox: chain focus is on something else,
        // or the list was rebuilt since the rebind. The engine's selection
        // stays as it is.
        acclog::Write("Menus.Inventory",
                      "SyncListBoxSelection: row=%p not in item_listbox "
                      "(size=%d); selection untouched",
                      row, acc::engine::ReadListBoxRowCount(lb));
        return;
    }
    const int before = acc::engine::ReadListBoxSelection(lb);
    __try {
        // playSound=0: the row name was already announced on the chain step,
        // and the activate about to follow plays its own item sound.
        reinterpret_cast<PFN_CSWGuiListBoxSetSelectedControl>(
            kAddrCSWGuiListBoxSetSelectedControl)(lb, idx, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Menus.Inventory",
                      "SyncListBoxSelection SEH (panel=%p row=%p)", panel, row);
        return;
    }
    acclog::Write("Menus.Inventory", "SyncListBoxSelection: row=%p idx=%d sel %d->%d",
                  row, idx, before, acc::engine::ReadListBoxSelection(lb));
}

void ForceRepopulate(void* panel) {
    if (!panel) return;
    if (!acc::addr::Ok(kAddrInventoryPopulateItemListBox)) {
        acclog::Write("Menus.Inventory",
                      "ForceRepopulate skipped (unresolved on build %s)",
                      acc::game::TitleName());
        return;
    }
    __try {
        auto fn = reinterpret_cast<PFN_PanelThiscall>(
            kAddrInventoryPopulateItemListBox);
        fn(panel);
        acclog::Write("Menus.Inventory",
                      "ForceRepopulate: PopulateItemListBox(panel=%p)", panel);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Menus.Inventory",
                      "ForceRepopulate SEH (panel=%p)", panel);
    }
}

}  // namespace acc::menus::inventory
