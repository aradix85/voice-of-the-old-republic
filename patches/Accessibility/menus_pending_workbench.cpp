// The workbench's deferred ops: selecting an upgrade slot, installing or
// removing an upgrade (two steps on KOTOR 1, one on KOTOR 2), and closing
// the mod picker.

#include "menus_pending_internal.h"

#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_listbox.h"
#include "speech.h"
#include "strings.h"

namespace acc::menus::pending {

namespace {

// Sentinel returned by ReadUpgradeSlotInstalled when the field can't be read
// (bad slot pointer / SEH fault) — distinct from nullptr (slot empty).
void* const kUpgradeSlotUnknown = reinterpret_cast<void*>(~uintptr_t(0));

// K2: the installed-mod pointer (field35) of the currently-open upgrade slot.
// The panel holds the active slot button at +0x3dbc; its custom_value indexes
// field35. SEH-guarded, in its own function so the SEH-using Drain stays free
// of unwinding objects (MSVC C2712). Returns kUpgradeSlotUnknown on any fault.
void* ReadUpgradeSlotInstalled(void* panel) {
    if (!panel) return kUpgradeSlotUnknown;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(panel);
        void* slotBtn = *reinterpret_cast<void**>(
            base + kUpgradePanelActiveSlotPtrOff);
        if (!slotBtn) return kUpgradeSlotUnknown;
        int cv = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(slotBtn) + kUpgradeSlotCustomValueOff);
        if (cv < 0 || cv > 5) return kUpgradeSlotUnknown;
        return *reinterpret_cast<void**>(
            base + kUpgradeSlotInstalledItemsOff + cv * 4);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return kUpgradeSlotUnknown;
    }
}

// K2: true iff `row` is the engine's "-" none/remove picker entry (key -1).
bool IsUpgradePickerNoneRow(void* row) {
    if (!row) return false;
    __try {
        return *reinterpret_cast<int32_t*>(
            reinterpret_cast<unsigned char*>(row) + kUpgradePickerRowKeyOff) == -1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Rows in the panel's LB_ITEMS (the control with id 0).
int UpgradeItemListRows(void* panel) {
    auto* controls = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(panel) + kPanelControlsOffset);
    if (!controls->data) return 0;
    for (int i = 0; i < controls->size; ++i) {
        void* c = controls->data[i];
        if (!c) continue;
        if (acc::engine::ReadControlId(c) != 0) continue;
        const int rows = acc::engine::ReadListBoxRowCount(c);
        return rows > 0 ? rows : 0;
    }
    return 0;
}

// The non-saber slot click installed, removed or found nothing: say so,
// with the slot's type name from the engine's strref table, so the user
// hears WHICH slot they acted on rather than a position number.
void AnnounceSlotOutcome(int slotIdx, int saberFlag, void* prevItem, void* newItem) {
    acc::strings::Id outcome = acc::strings::Id::Count_;
    const char* outcomeTag = "no-change";
    if (prevItem == nullptr && newItem != nullptr) {
        outcome = acc::strings::Id::WorkbenchSlotInstalled;
        outcomeTag = "installed";
    } else if (prevItem != nullptr && newItem == nullptr) {
        outcome = acc::strings::Id::WorkbenchSlotRemoved;
        outcomeTag = "removed";
    } else if (prevItem == nullptr && newItem == nullptr) {
        outcome = acc::strings::Id::WorkbenchSlotNoMatch;
        outcomeTag = "no-match";
    }
    char slotName[128] = {0};
    int upgradeType = -1;
    uint32_t strref = 0;
    // Per-game index + table read; see LookupUpgradeSlotType.
    if (acc::engine::LookupUpgradeSlotType(slotIdx, saberFlag, &upgradeType, &strref)) {
        acc::engine::LookupTlk(strref, slotName, sizeof(slotName));
        acclog::Write("WorkbenchSlotSelect",
                      "non-saber path (saber=%d slot_idx=%d "
                      "upgrade_type=%d strref=%u name=\"%s\") -> %s; picker disarmed",
                      saberFlag, slotIdx, upgradeType, strref, slotName, outcomeTag);
    } else {
        acclog::Write("WorkbenchSlotSelect",
                      "non-saber path (saber=%d slot_idx=%d "
                      "no table entry) -> %s; picker disarmed",
                      saberFlag, slotIdx, outcomeTag);
    }
    if (outcome == acc::strings::Id::Count_) return;
    const char* result = acc::strings::Get(outcome);
    char msg[256];
    if (slotName[0] != '\0') {
        snprintf(msg, sizeof(msg), "%s, %s", slotName, result);
    } else {
        snprintf(msg, sizeof(msg), "%s", result);
    }
    acc::speech::Speak(msg, /*interrupt=*/false);
}

}  // namespace

// Workbench slot activation. Mirrors EquipSelect's shape but against
// the RE'd CSWGuiUpgrade slot-pick chain at 0x006c3c30 (OnEnterSlot)
// and 0x006c6500 (OnSlotSelected). Direct call bypasses click-sim
// because the upgrade.gui labels at IDs 5..11 cover the slot buttons
// at IDs 12..18 in z-order — MoveMouseToPosition's hit-test resolves
// mouseOver to a label, so the engine's mouse pipeline never reaches
// the slot button. OnSlotSelected is the function that populates
// LB_ITEMS with the inventory mods compatible with the slot.
void DrainWorkbenchSlotSelect(const PendingOp& op) {
    void* panel   = op.a;
    void* slotBtn = op.b;
    if (panel && slotBtn) {
        uint32_t prevIsActive = RaiseIsActiveIfZero(slotBtn);
        auto onEnter  = reinterpret_cast<PFN_CSWGuiUpgradeOnEnterSlot>(
            kAddrCSWGuiUpgradeOnEnterSlot);
        auto onSelect = reinterpret_cast<PFN_CSWGuiUpgradeOnSlotSelected>(
            kAddrCSWGuiUpgradeOnSlotSelected);
        acclog::Write("Update", "WorkbenchSlotSelect panel=%p slot=%p is_active=%u%s",
                      panel, slotBtn, prevIsActive, RaiseNote(prevIsActive));

        // Slot index lives at slot_btn+0x58 (set by OnPanelAdded). Engine
        // reads it via `edi = [ebx+0x58]` at OnSlotSelected+0x46. The
        // installed-upgrade pointer for each slot lives at
        // panel.field35_0x2f74[slot_idx*4] (4-byte CSWSItem*); the
        // non-saber install path mutates this from 0 → item on install
        // and item → 0 on remove (verified in OnSlotSelected non-saber
        // branch and the InsertUpgrade path).
        int slotIdx = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(slotBtn) + kUpgradeSlotCustomValueOff);
        void** field35Slot = reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(panel) + kUpgradeSlotInstalledItemsOff + slotIdx * 4);
        void* prevItem = *field35Slot;

        onEnter(panel, slotBtn);
        onSelect(panel, slotBtn);

        void* newItem = *field35Slot;

        // Post-call: decide whether the picker should stay armed.
        // OnSlotSelected branches on panel.field25_0x2f4c:
        //   * value 1 (true lightsaber): builds compatible-mods list
        //     from upcrystals_2da / upgrades_2da and AddControls-
        //     replaces LB_ITEMS — picker UX engages.
        //   * value 2/3/4 (4-slot / 3-slot / 2-slot non-saber categories):
        //     directly installs the upgrade staged by OnPanelAdded
        //     (panel.field_0x2f84[slot_idx] tag lookup against party
        //     inventory), or removes the existing upgrade if the slot
        //     was already filled. LB_ITEMS is NOT populated.
        //
        // If LB_ITEMS row count is still 0 after dispatch, we're in
        // the non-saber install/remove path — disarm the picker and
        // announce the outcome by comparing field35_0x2f74[slot_idx]
        // before vs after.
        uint8_t saberFlag = *(reinterpret_cast<unsigned char*>(panel) + kUpgradePanelCategoryOff);
        const int lbRows = UpgradeItemListRows(panel);

        acclog::Write("Update", "WorkbenchSlotSelect done panel=%p slot=%p "
                      "saber=%d slot_idx=%d field35: %p -> %p lb_rows=%d",
                      panel, slotBtn, (int)saberFlag, slotIdx,
                      prevItem, newItem, lbRows);

        if (lbRows == 0) {
            acc::menus::listbox::ClearWorkbenchUpgradeArmLatch();
            AnnounceSlotOutcome(slotIdx, (int)saberFlag, prevItem, newItem);
        }
    }
}

// Workbench upgrade commit. Two-step via direct engine dispatch on
// the RE'd OnUpgradeSelected (row-stage) + OnAssemble (install).
// OnAssemble calls FinishUpgrading on the parent UpgradeItemSelect
// and PopModalPanel — the upgrade panel closes synchronously when
// this returns. Mirrors EquipCommit's row-then-OK shape.
void DrainWorkbenchUpgradeCommit(const PendingOp& op) {
    void* panel = op.a;
    void* row   = op.b;
    void* btn   = op.c;
    if (panel && row && btn) {
        uint32_t prevRowActive = RaiseIsActiveIfZero(row);
        uint32_t prevBtnActive = RaiseIsActiveIfZero(btn);
        auto onRow = reinterpret_cast<PFN_CSWGuiUpgradeOnUpgradeSelected>(
            kAddrCSWGuiUpgradeOnUpgradeSelected);
        auto onAsm = reinterpret_cast<PFN_CSWGuiUpgradeOnAssemble>(
            kAddrCSWGuiUpgradeOnAssemble);
        acclog::Write("Update", "WorkbenchUpgradeCommit panel=%p row=%p btn=%p "
                      "row.is_active=%u%s btn.is_active=%u%s",
                      panel, row, btn,
                      prevRowActive, RaiseNote(prevRowActive),
                      prevBtnActive, RaiseNote(prevBtnActive));
        onRow(panel, row);
        onAsm(panel, btn);
        acclog::Write("Update", "WorkbenchUpgradeCommit done panel=%p row=%p btn=%p",
                      panel, row, btn);
    } else {
        acclog::Write("Update", "WorkbenchUpgradeCommit panel=%p row=%p btn=%p (skipped)",
                      panel, row, btn);
    }
}

// K2 upgrade install/remove — ONE step. On K2 the picker row's own
// activate handler (CSWGuiUpgrade::OnUpgradeSelected @0x008cdb00) does the
// whole job itself: it reads the row's obj id (+0x1d0), and if it resolves
// to an inventory mod it installs it into the slot (removing+returning any
// existing one first); if the row is the engine's "-" none entry
// (obj id 0x7f000000, key +0x1e0 == -1) it just removes the current mod.
// It self-guards greyed rows (routes them to the can't-select stub) and
// calls CloseItems at the end, closing the picker and refocusing the slot.
// So there is NO separate assemble step per pick — that is what OnAssemble
// (Assemble) does, and firing it here with K1's button id pops the whole
// panel back to the item list without installing. K1 keeps its two-step
// WorkbenchUpgradeCommit above.
void DrainWorkbenchUpgradeInstallK2(const PendingOp& op) {
    void* panel = op.a;
    void* row   = op.b;
    if (panel && row) {
        // Read the currently-open slot's installed-mod pointer (field35)
        // before and after the engine call, so we announce what actually
        // happened rather than assuming success — an incompatible slot/mod
        // leaves field35 unchanged and must not say "installed".
        bool isNoneRow = IsUpgradePickerNoneRow(row);
        void* before = ReadUpgradeSlotInstalled(panel);

        uint32_t prevRowActive = RaiseIsActiveIfZero(row);
        auto onRow = reinterpret_cast<PFN_CSWGuiUpgradeOnUpgradeSelected>(
            kAddrCSWGuiUpgradeOnUpgradeSelected);
        onRow(panel, row);

        void* after = ReadUpgradeSlotInstalled(panel);

        acc::strings::Id say = acc::strings::Id::Count_;
        if (before != kUpgradeSlotUnknown && after != kUpgradeSlotUnknown) {
            if (after != nullptr && after != before) {
                say = acc::strings::Id::WorkbenchSlotInstalled;
            } else if (after == nullptr && before != nullptr) {
                say = acc::strings::Id::WorkbenchSlotRemoved;
            } else if (!isNoneRow) {
                // Real mod, nothing changed → engine refused (slot/mod
                // incompatible, requirements unmet). The none row on an
                // already-empty slot legitimately changes nothing — stay
                // silent there and let the slot re-announce.
                say = acc::strings::Id::WorkbenchUpgradeFailed;
            }
        }
        if (say != acc::strings::Id::Count_) {
            acc::speech::Speak(acc::strings::Get(say), /*interrupt=*/false);
        }
        acclog::Write("Update", "WorkbenchUpgradeInstallK2 done panel=%p row=%p "
                      "none=%d before=%p after=%p row.is_active=%u%s -> say=%d",
                      panel, row, isNoneRow ? 1 : 0, before, after,
                      prevRowActive, RaiseNote(prevRowActive),
                      (int)say);
    } else {
        acclog::Write("Update", "WorkbenchUpgradeInstallK2 panel=%p row=%p (skipped)",
                      panel, row);
    }
}

// Back out of the mod-picker without committing (Esc). OnSlotSelected's
// ShowItems(1) disabled the sibling slot buttons; the engine only undoes
// that on commit. Mirror its close tail so the slots stop reading
// "unavailable": ShowItems(0) re-enables them (and SetActiveControl's back
// to the slot zone), then clear the panel's "picker open" bit.
void DrainWorkbenchPickerCancel(const PendingOp& op) {
    void* panel = op.a;
    if (panel) {
        auto showItems = reinterpret_cast<PFN_CSWGuiUpgradeShowItems>(
            kAddrCSWGuiUpgradeShowItems);
        showItems(panel, 0);
        uint32_t* flag = reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(panel) + kUpgradePickerOpenFlagOff);
        *flag &= ~1u;
        acclog::Write("Update", "WorkbenchPickerCancel panel=%p (ShowItems(0) + "
                      "cleared picker-open bit)", panel);
    } else {
        acclog::Write("Update", "WorkbenchPickerCancel panel=%p (skipped)", panel);
    }
}

}  // namespace acc::menus::pending
