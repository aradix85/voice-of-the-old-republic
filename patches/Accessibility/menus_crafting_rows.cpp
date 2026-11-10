// Rows of the K2 workbench and crafting lists: the row commit (selection,
// is_active and the panel's own commit button), the cost or yield suffix
// on a chain step, and the chain focus mirrored into the engine selection.

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "menus_crafting_internal.h"

#include "engine_game.h"     // IsKotor2: the WorkbenchSelect commit is K2-shaped
#include "engine_offsets.h"
#include "engine_panels.h"   // IdentifyPanel, PanelKind
#include "engine_reads.h"    // ResolveItemFromClientHandle
#include "log.h"
#include "menus_chain.h"
#include "menus_internal.h"  // the ctor-bound control resolvers
#include "menus_pending.h"   // RaiseIsActiveIfZero
#include "speech.h"
#include "strings.h"

using acc::engine::IdentifyPanel;
using acc::engine::PanelKind;
using acc::engine::TryReadU32;

namespace acc::menus::crafting {

namespace {

using PFN_ControlHandleInputEvent = void(__thiscall*)(void* self, int code, int state);
using PFN_CraftCost = unsigned int(__thiscall*)(void* self, void* item);
constexpr int kVtableHandleInputEvent = 15;

// Index of `row` in the listbox's controls array, or -1: a pointer
// comparison, nothing is read through the rows.
int RowIndexIn(void* listBox, void* row) {
    if (!listBox || !row) return -1;
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            static_cast<unsigned char*>(listBox) + kListBoxControlsOffset);
        if (!list->data || list->size <= 0) return -1;
        const int n = list->size > 4096 ? 4096 : list->size;
        for (int i = 0; i < n; ++i) {
            if (list->data[i] == row) return i;
        }
        return -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// Makes `row` the selected row: what a mouse click leaves behind, minus the
// click. Both halves matter:
//   * selection_index: BTN_Accept (component 0x008D2150, chemical
//     0x008D7A80) takes its row from GetSelectedRow (0x0041FFB0), which is
//     GetRow(selection_index), and GetRow indexes the same controls array
//     RowIndexIn scans.
//   * is_active: all four workers behind Accept (component create 0x008D4AC0
//     / breakdown 0x008D4680, chemical create 0x008D98F0 / breakdown
//     0x008D95E0) open with "if (row->is_active == 0) return;" and decline
//     silently. The listbox raises it on the row's mouse-enter, which the
//     keyboard never produces: that was the whole "nothing can be crafted"
//     bug.
// False when a write faulted.
bool SelectRow(void* listBox, void* row, int index) {
    if (!listBox || index < 0) return false;
    __try {
        *reinterpret_cast<short*>(static_cast<unsigned char*>(listBox) +
                                  kListBoxSelectionIndexOffset) = static_cast<short>(index);
        acc::menus::pending::RaiseIsActiveIfZero(row);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return true;
}

// The CSWSItem* behind a row: the same two hops the engine's create and
// breakdown workers take on the row's object id.
void* ResolveRowItem(void* row) {
    uint32_t handle = 0;
    if (!TryReadU32(row, kStoreItemEntryObjIdOffset, &handle) || !handle) return nullptr;
    return acc::engine::ResolveItemFromClientHandle(handle);
}

// The engine's own "can make this" gate; true when unreadable, so a bad read
// never invents a refusal.
bool IsRowCraftable(void* row) {
    uint32_t flag = 0;
    if (!acc::off::Ok(kCraftRowCraftableFlagOffset) ||
        !TryReadU32(row, kCraftRowCraftableFlagOffset, &flag)) {
        return true;
    }
    return flag != 0;
}

// The engine's cost function (the two screens use different ones, see
// kAddrCraftComponentCost); 0 on a fault.
unsigned int CallCraftCost(uintptr_t fnAddr, void* panel, void* item) {
    if (!fnAddr || !panel || !item) return 0;
    __try {
        return reinterpret_cast<PFN_CraftCost>(fnAddr)(panel, item);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// The panel's ctor-cached skill factor, or 0 for a fault or a value outside
// (0, 1]: a yield that cannot be trusted is not quoted.
float ReadSkillFactor(void* panel, bool medical) {
    const size_t off = medical ? kCraftChemicalSkillFactorOffset : kCraftComponentSkillFactorOffset;
    uint32_t bits = 0;
    if (!acc::off::Ok(off) || !TryReadU32(panel, off, &bits)) return 0.0f;
    float v = 0.0f;
    memcpy(&v, &bits, sizeof(v));
    return (v > 0.0f && v <= 1.0f) ? v : 0.0f;
}

// The control's own HandleInputEvent (vtable slot 15) with the activate
// code, as a click would.
bool FireActivate(void* control) {
    if (!control) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(control);
        auto fn = reinterpret_cast<PFN_ControlHandleInputEvent>(vt[kVtableHandleInputEvent]);
        fn(control, 0x27, 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

bool ResolveRowCommit(void* panel, void* control, void** outListBox) {
    if (!panel || !control || !outListBox) return false;
    const PanelKind pk = IdentifyPanel(panel);
    void* lb = nullptr;
    if (pk == PanelKind::WorkbenchSelect && acc::game::IsKotor2()) {
        lb = acc::menus::detail::UpgradeSelPanelListBox(panel);  // K1's upgradesel has no list
    } else if (IsCraftingKind(pk)) {
        lb = FindActiveCraftList(panel);
    } else {
        return false;
    }
    if (!lb || RowIndexIn(lb, control) < 0) return false;
    *outListBox = lb;
    return true;
}

void DispatchRowCommit(void* panel, void* listBox, void* row) {
    // A tick has passed since the key: the list may have been repopulated
    // (a category switch racing the Enter), so a vanished row is a no-op.
    const int idx = RowIndexIn(listBox, row);
    if (idx < 0) {
        acclog::Write("Crafting", "row-commit dropped: row=%p no longer in lb=%p (panel=%p)",
                      row, listBox, panel);
        return;
    }
    if (!SelectRow(listBox, row, idx)) {
        acclog::Write("Crafting", "row-commit dropped: selection write failed lb=%p idx=%d",
                      listBox, idx);
        return;
    }
    // The panel's own commit button: BTN_UPGRADEITEMS on the workbench top
    // screen, BTN_Accept on the crafting screens. It gates on is_active like
    // its row (as every hand-fired dispatch in menus_pending does).
    const bool isSel = IdentifyPanel(panel) == PanelKind::WorkbenchSelect;
    void* btn = isSel ? acc::menus::detail::UpgradeSelPanelUpgradeButton(panel)
                      : acc::menus::detail::CraftPanelAcceptButton(panel);
    if (!btn) {
        acclog::Write("Crafting", "row-commit: commit button not resolved on panel=%p", panel);
        return;
    }
    __try {
        acc::menus::pending::RaiseIsActiveIfZero(btn);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    const bool ok = FireActivate(btn);
    acclog::Write("Crafting", "row-commit panel=%p lb=%p row=%p idx=%d btn=%p fired=%d",
                  panel, listBox, row, idx, btn, ok ? 1 : 0);
    // Opened from the list by the user: Tick must not bounce the arriving
    // upgrade screen. About 20 ticks covers the frame or two before it
    // fronts.
    if (ok && isSel) g_userOpenedUpgradeTtl = 20;
}

void AnnounceChainStepSuffix(void* panel, void* control) {
    if (!panel || !control || !acc::game::IsKotor2()) return;
    const PanelKind pk = IdentifyPanel(panel);
    if (!IsCraftingKind(pk)) return;
    // Rows of the actionable list only; buttons and the resource row fall out.
    void* lb = FindActiveCraftList(panel);
    if (!lb || RowIndexIn(lb, control) < 0) return;
    void* item = ResolveRowItem(control);
    if (!item) {
        acclog::Write("Crafting", "chain-step suffix row=%p: item resolve failed", control);
        return;
    }
    const bool medical = pk == PanelKind::WorkbenchCreateMedical;
    const unsigned int cost =
        CallCraftCost(medical ? kAddrCraftChemicalCost : kAddrCraftComponentCost, panel, item);
    if (cost == 0) return;  // a fault, or an item the engine prices at nothing

    // The create view quotes the cost, the breakdown view the return: the
    // cost scaled by the bench skill, at least 1, which is why a low-skill
    // character loses material on every teardown.
    const bool breakdown = acc::menus::detail::CraftPanelShopListBox(panel) != lb;
    char msg[96];
    if (breakdown) {
        const float factor = ReadSkillFactor(panel, medical);
        if (factor <= 0.0f) return;
        int yield = static_cast<int>(static_cast<float>(cost) * factor);
        if (yield < 1) yield = 1;
        snprintf(msg, sizeof(msg), acc::strings::Get(acc::strings::Id::FmtCraftYield), yield);
    } else {
        snprintf(msg, sizeof(msg), acc::strings::Get(acc::strings::Id::FmtCraftCost),
                 static_cast<int>(cost));
    }
    // The list shows recipes up to eight ranks above the character, and the
    // engine answers a press on one with only a message box.
    const bool craftable = IsRowCraftable(control);
    if (!craftable) {
        const size_t used = strlen(msg);
        snprintf(msg + used, sizeof(msg) - used, "%s",
                 acc::strings::Get(acc::strings::Id::DisabledSuffix));
    }
    acc::speech::Speak(msg, /*interrupt=*/false);
    acclog::Write("Crafting", "chain-step suffix row=%p item=%p %s cost=%u craftable=%d -> \"%s\"",
                  control, item, breakdown ? "breakdown" : "create", cost, craftable ? 1 : 0, msg);
}

void SyncSelectedRowFromChainFocus() {
    using namespace acc::menus::chain;
    if (!acc::game::IsKotor2()) return;
    void* panel = g_chainPanel;
    if (!panel || !IsCraftingKind(IdentifyPanel(panel))) return;
    if (g_chainIndex < 0 || g_chainIndex >= g_chainCount) return;
    void* focused = g_chain[g_chainIndex].control;
    void* lb = FindActiveCraftList(panel);
    if (!lb || !focused) return;
    const int idx = RowIndexIn(lb, focused);
    if (idx < 0) return;  // a button or category, not an item row
    static void* s_lastRow = nullptr;
    if (!SelectRow(lb, focused, idx) || focused == s_lastRow) return;
    s_lastRow = focused;
    acclog::Write("Crafting", "chain focus -> engine selection panel=%p lb=%p row=%p idx=%d",
                  panel, lb, focused, idx);
}

}  // namespace acc::menus::crafting
