// Rows of the K2 workbench and crafting lists: the row commit (selection,
// is_active and the panel's own commit button), the cost or yield suffix
// on a chain step, and the chain focus mirrored into the engine selection.

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "menus_crafting_internal.h"
#include "engine_game.h"      // IsKotor2 — WorkbenchSelect row commit is K2-only
#include "engine_manager.h"   // GetForegroundPanel, kAddrGuiManagerPtr
#include "engine_offsets.h"
#include "engine_panels.h"    // IdentifyPanel, PanelKind
#include "engine_reads.h"     // ResolveItemFromClientHandle — row -> CSWSItem*
#include "log.h"
#include "menus_chain.h"      // RebindChainPreserveIndex on list changes
#include "menus_pending.h"    // QueueActivate — auto-open redirect (BTN_BACK)
#include "menus_extract.h"    // FromControl — title label read
#include "hotkeys.h"          // Pressed() for Q/E (CraftViewToggle)
#include "menus_internal.h"   // detail::FindControlById
#include "strings.h"
#include "speech.h"

using acc::engine::IdentifyPanel;
using acc::engine::ReadListBoxRowCount;
using acc::engine::PanelKind;
using acc::menus::detail::FindControlById;

namespace acc::menus::crafting {

namespace {

constexpr int kVtableHandleInputEvent = 15;
typedef void (__thiscall* PFN_ControlHandleInputEvent)(void* this_, int code,
                                                       int state);

// Index of `row` in the listbox's controls array, or -1. SEH-guarded
// pointer-equality scan — no reads through the rows themselves.
int RowIndexIn(void* listBox, void* row) {
    if (!listBox || !row) return -1;
    __try {
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(listBox) + kListBoxControlsOffset);
        if (!list->data || list->size <= 0) return -1;
        int n = list->size > 4096 ? 4096 : list->size;
        for (int i = 0; i < n; ++i) {
            if (list->data[i] == row) return i;
        }
        return -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool WriteListBoxSelection(void* listBox, int index) {
    if (!listBox || index < 0) return false;
    __try {
        *reinterpret_cast<short*>(
            reinterpret_cast<unsigned char*>(listBox) +
            kListBoxSelectionIndexOffset) = (short)index;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Make `row` the engine's selected row of `listBox`: what a mouse click on
// the row leaves behind, minus the click.
//
// Both halves are load-bearing, and the second one is what the screen was
// missing (2026-08-13, kotor2 Ghidra):
//
//   * selection_index — BTN_Accept's handler (component 0x008D2150 /
//     chemical 0x008D7A80) fetches the row to act on with
//     CSWGuiListBox::GetSelectedRow (0x0041FFB0), which is literally
//     `GetRow(this->selection_index)`; GetRow (0x0041FF70) indexes the same
//     controls array RowIndexIn scans, so our index IS the engine's index.
//   * is_active — every one of the four worker functions the Accept handler
//     dispatches into (component create 0x008D4AC0 / breakdown 0x008D4680,
//     chemical create 0x008D98F0 / breakdown 0x008D95E0) opens with
//     `if (row->is_active == 0) return;` and returns SILENTLY. Normally the
//     listbox raises it by firing the row's HandleInputEvent(0) when the
//     mouse enters it; our keyboard path never generates that event (the
//     cursor warp's hit-test does not land on these rows), so the flag stayed
//     zero, Accept fired, and the engine declined without a sound. That is
//     the whole "nothing can be crafted" bug.
//
// Returns false only if the selection write itself faulted.
bool SelectRow(void* listBox, void* row, int index) {
    if (!WriteListBoxSelection(listBox, index)) return false;
    __try {
        acc::menus::pending::RaiseIsActiveIfZero(row);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return true;
}

// The CSWSItem* behind a crafting row, or null. Same two hops the engine's
// own create/breakdown workers take on the row's object id at +0x1d0.
void* ResolveRowItem(void* row) {
    if (!row) return nullptr;
    uint32_t handle = 0;
    __try {
        handle = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(row) +
            kStoreItemEntryObjIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    if (!handle) return nullptr;
    return acc::engine::ResolveItemFromClientHandle(handle);
}

// Can the character actually make (or break down) this row? Reads the
// engine's own gate; true on fault, so a bad read never invents a refusal.
bool IsRowCraftable(void* row) {
    if (!row || !acc::off::Ok(kCraftRowCraftableFlagOffset)) return true;
    __try {
        return *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(row) +
            kCraftRowCraftableFlagOffset) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

// Price quote from the engine's own cost function. The two crafting screens
// use different ones (see kAddrCraftComponentCost). 0 on fault.
typedef unsigned int (__thiscall* PFN_CraftCost)(void* this_, void* item);

unsigned int CallCraftCost(uintptr_t fnAddr, void* panel, void* item) {
    if (!fnAddr || !panel || !item) return 0;
    __try {
        auto fn = reinterpret_cast<PFN_CraftCost>(fnAddr);
        return fn(panel, item);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// The panel's ctor-cached skill factor. 0.0f on fault or a nonsense value —
// callers stay silent rather than quote a yield they can't trust.
float ReadSkillFactor(void* panel, bool medical) {
    size_t off = medical ? kCraftChemicalSkillFactorOffset
                         : kCraftComponentSkillFactorOffset;
    if (!acc::off::Ok(off)) return 0.0f;
    float v = 0.0f;
    __try {
        v = *reinterpret_cast<float*>(
            reinterpret_cast<unsigned char*>(panel) + off);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0.0f;
    }
    if (!(v > 0.0f) || v > 1.0f) return 0.0f;
    return v;
}

bool FireActivate(void* control) {
    if (!control) return false;
    __try {
        void** vt = *reinterpret_cast<void***>(control);
        auto fn = reinterpret_cast<PFN_ControlHandleInputEvent>(
            vt[kVtableHandleInputEvent]);
        fn(control, 0x27, 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

bool ResolveRowCommit(void* panel, void* control, void** outListBox) {
    if (!panel || !control || !outListBox) return false;
    PanelKind pk = IdentifyPanel(panel);
    void* lb = nullptr;
    if (pk == PanelKind::WorkbenchSelect && acc::game::IsKotor2()) {
        // K1's upgradesel has no listbox at all — this path is K2 shape.
        lb = acc::menus::detail::UpgradeSelPanelListBox(panel);
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
    // Re-verify at drain time — a tick has passed since the input event
    // and the engine may have repopulated the list (category switch racing
    // the Enter). A vanished row is a logged no-op, never a stale commit.
    int idx = RowIndexIn(listBox, row);
    if (idx < 0) {
        acclog::Write("Crafting",
                      "row-commit dropped: row=%p no longer in lb=%p (panel=%p)",
                      row, listBox, panel);
        return;
    }
    if (!SelectRow(listBox, row, idx)) {
        acclog::Write("Crafting",
                      "row-commit dropped: selection write failed lb=%p idx=%d",
                      listBox, idx);
        return;
    }
    // The commit button is the panel's own: BTN_UPGRADEITEMS on the
    // workbench top screen, BTN_Accept on the crafting screens. Resolved
    // from the ctor-bound member here at drain time, same freshness the
    // old .gui-id lookup had.
    bool isSel = IdentifyPanel(panel) == PanelKind::WorkbenchSelect;
    void* btn = isSel ? acc::menus::detail::UpgradeSelPanelUpgradeButton(panel)
                      : acc::menus::detail::CraftPanelAcceptButton(panel);
    if (!btn) {
        acclog::Write("Crafting",
                      "row-commit: commit button not resolved on panel=%p",
                      panel);
        return;
    }
    // The commit button gates the same way its row does; every other
    // hand-fired dispatch in the mod raises both (see EquipCommit /
    // WorkbenchUpgradeCommit in menus_pending.cpp).
    __try {
        acc::menus::pending::RaiseIsActiveIfZero(btn);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    bool ok = FireActivate(btn);
    acclog::Write("Crafting",
                  "row-commit panel=%p lb=%p row=%p idx=%d btn=%p fired=%d",
                  panel, listBox, row, idx, btn, ok ? 1 : 0);
    // Opening the upgrade flow from the list is user-initiated — mark it so
    // the arriving CSWGuiUpgrade edge is NOT bounced back to the list (see
    // the auto-open redirect in Tick). ~20 ticks covers the engine's
    // one-to-two-frame delay before the panel actually fronts.
    if (ok && isSel) {
        g_userOpenedUpgradeTtl = 20;
    }
}

void AnnounceChainStepSuffix(void* panel, void* control) {
    if (!panel || !control || !acc::game::IsKotor2()) return;
    PanelKind pk = IdentifyPanel(panel);
    if (!IsCraftingKind(pk)) return;

    // Only rows of the list the user can act on. Buttons and the resource
    // row fall out here.
    void* lb = FindActiveCraftList(panel);
    if (!lb || RowIndexIn(lb, control) < 0) return;

    void* item = ResolveRowItem(control);
    if (!item) {
        acclog::Write("Crafting",
                      "chain-step suffix row=%p: item resolve failed", control);
        return;
    }

    bool medical = (pk == PanelKind::WorkbenchCreateMedical);
    unsigned int cost = CallCraftCost(
        medical ? kAddrCraftChemicalCost : kAddrCraftComponentCost,
        panel, item);
    if (cost == 0) return;  // fault, or an item the engine prices at nothing

    // Which list is showing decides which number is the useful one: what it
    // costs to build, or what breaking it down hands back. They are not the
    // same — the return is the cost scaled by the bench skill, min 1, which
    // is why a low-skill character loses material on every teardown.
    bool breakdown = (acc::menus::detail::CraftPanelShopListBox(panel) != lb);
    char msg[96];
    if (breakdown) {
        float factor = ReadSkillFactor(panel, medical);
        if (factor <= 0.0f) return;
        int yield = (int)((float)cost * factor);
        if (yield < 1) yield = 1;
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtCraftYield), yield);
    } else {
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtCraftCost), (int)cost);
    }
    // The list shows recipes up to eight skill ranks above the character, so
    // "you can't make this one yet" is ordinary and has to be audible — the
    // engine answers a press on such a row with a message box and nothing
    // else. Same wording the mod already uses for unavailable buttons.
    bool craftable = IsRowCraftable(control);
    if (!craftable) {
        size_t used = strlen(msg);
        snprintf(msg + used, sizeof(msg) - used, "%s",
                 acc::strings::Get(acc::strings::Id::DisabledSuffix));
    }
    acc::speech::Speak(msg, /*interrupt=*/false);
    acclog::Write("Crafting",
                  "chain-step suffix row=%p item=%p %s cost=%u craftable=%d "
                  "-> \"%s\"",
                  control, item, breakdown ? "breakdown" : "create", cost,
                  craftable ? 1 : 0, msg);
}

void SyncSelectedRowFromChainFocus() {
    if (!acc::game::IsKotor2()) return;
    void* panel = acc::menus::chain::g_chainPanel;
    if (!panel || !IsCraftingKind(IdentifyPanel(panel))) return;
    if (acc::menus::chain::g_chainIndex < 0 ||
        acc::menus::chain::g_chainIndex >= acc::menus::chain::g_chainCount) {
        return;
    }
    void* focused =
        acc::menus::chain::g_chain[acc::menus::chain::g_chainIndex].control;
    void* lb = FindActiveCraftList(panel);
    if (!lb || !focused) return;
    int idx = RowIndexIn(lb, focused);
    if (idx < 0) return;  // focus is on a button / category, not an item row

    static void* s_lastRow = nullptr;
    if (!SelectRow(lb, focused, idx)) return;
    if (focused != s_lastRow) {
        s_lastRow = focused;
        acclog::Write("Crafting",
                      "chain focus -> engine selection panel=%p lb=%p row=%p "
                      "idx=%d", panel, lb, focused, idx);
    }
}

}  // namespace acc::menus::crafting
