// The equipment screen's deferred ops: opening a slot's item picker,
// equipping the chosen row, and the engine's own cancel.

#include "menus_pending_internal.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "log.h"

namespace acc::menus::pending {

// Equip-screen slot activation. Mirrors the engine's mouse-driven path:
//   1. Raise slot_btn->is_active = 1 (LMouseDown sets this flag normally;
//      OnSelectSlot's prologue gates on it and returns early if zero).
//   2. OnEnterSlot(panel, slot_btn) — populates panel.items_listbox with
//      items matching the slot's type. Sets panel.selected_slot.
//   3. OnSelectSlot(panel, slot_btn) — if items_listbox.size > 1, stages
//      the equip (raises panel.field33_0x4270 |= 1, pre-selects row 1,
//      shows description). If size == 1 (only protoitem template), pops
//      the engine's empty-slot modal — which is the correct
//      behaviour for a slot the player has no fitting items for.
//
// Dispatched here (not synchronously from the input hook) for the same
// reason MoveMouseToPosition is — these functions reach deep into GUI
// state that's mid-update during input dispatch.
void DrainEquipSelect(const PendingOp& op) {
    void* panel   = op.a;
    void* slotBtn = op.b;
    if (panel && slotBtn) {
        uint32_t prevIsActive = RaiseIsActiveIfZero(slotBtn);
        auto onEnter  = reinterpret_cast<PFN_InGameEquipOnEnterSlot>(
            kAddrInGameEquipOnEnterSlot);
        auto onSelect = reinterpret_cast<PFN_InGameEquipOnSelectSlot>(
            kAddrInGameEquipOnSelectSlot);
        acclog::Write("Update", "EquipSelect panel=%p slot=%p is_active=%u%s",
                      panel, slotBtn, prevIsActive, RaiseNote(prevIsActive));
        onEnter(panel, slotBtn);
        onSelect(panel, slotBtn);
        acclog::Write("Update", "EquipSelect done panel=%p slot=%p", panel, slotBtn);
    }
}

// Equip-row commit. OnItemSelected is the function that calls EquipItem
// and actually equips the item; OnOKPressed is just cleanup (closes the
// description panel, clears previously_equipped_*). Both gates the
// engine cares about (description_listbox.bit_flags & 2,
// items_listbox.bit_flags & 8, panel.field33_0x4270 & 1) were raised
// earlier by OnSelectSlot — we just need to raise row->is_active and
// btn_equip->is_active before invoking, mirroring what HandleLMouseDown
// would do in mouse-driven play.
void DrainEquipCommit(const PendingOp& op) {
    void* panel = op.a;
    void* row   = op.b;
    void* btn   = op.c;
    if (panel && row && btn) {
        uint32_t prevRowActive = RaiseIsActiveIfZero(row);
        uint32_t prevBtnActive = RaiseIsActiveIfZero(btn);
        auto onItem = reinterpret_cast<PFN_InGameEquipOnItemSelected>(
            kAddrInGameEquipOnItemSelected);
        auto onOK   = reinterpret_cast<PFN_InGameEquipOnOKPressed>(
            kAddrInGameEquipOnOKPressed);
        acclog::Write("Update", "EquipCommit panel=%p row=%p btn=%p "
                      "row.is_active=%u%s btn.is_active=%u%s",
                      panel, row, btn,
                      prevRowActive, RaiseNote(prevRowActive),
                      prevBtnActive, RaiseNote(prevBtnActive));
        onItem(panel, row);
        onOK(panel, btn);
        acclog::Write("Update", "EquipCommit done panel=%p row=%p btn=%p",
                      panel, row, btn);
    }
}

// Equip-picker cancel — the engine's own "back out of the item zone".
//
// This one needs no direct handler address: CSWGuiInGameEquip::HandleInputEvent
// already implements the whole cancel, and it is one switch case away from
// the code the user's Escape key produces. With the picker-open bit set,
// case 0x28 restores previously_equipped_id / previously_equipped_item (and
// the off-hand weapon a two-hander displaced), releases both remembered
// copies, and calls CloseDescription — which runs ShowDescription(0), the
// ONLY thing that re-raises the interactive bit on all nine slot buttons and
// labels, clears the picker-open bit, re-focuses the slot button and re-runs
// OnEnterSlot. With the bit clear the same case closes the whole equipment
// screen instead, so this must only ever be queued while the picker is up.
//
// Why this matters beyond the announcement: browsing the list EQUIPS as it
// goes (OnItemSelected is the row's select handler and calls EquipItem
// directly — the engine's live preview, same as mouse hover). Without the
// engine's cancel, an Escape left whatever the last arrow landed on
// permanently worn, and left every slot reading "unavailable" for the rest
// of the panel's life, on both games.
//
// ManagerTranslateCode rather than a literal 0x28: it is our port of the
// engine's own logical->InputIndices switch, verified on both games, and
// it keeps the cancel keyed to the same constant the input log prints.
void DrainEquipPickerCancel(const PendingOp& op) {
    void* panel = op.a;
    auto fn = acc::engine::ReadHandleInputEvent(panel);
    if (!fn) return;
    int code = acc::engine::ManagerTranslateCode(kInputEsc1);
    acclog::Write("Update", "EquipPickerCancel panel=%p "
                  "HandleInputEvent(code=0x%x) (engine revert + "
                  "CloseDescription)", panel, code);
    fn(panel, code, 1);
    acclog::Write("Update", "EquipPickerCancel done panel=%p", panel);
}

}  // namespace acc::menus::pending
