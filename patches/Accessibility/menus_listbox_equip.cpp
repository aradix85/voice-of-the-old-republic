// The equipment picker: the list of items for a slot, equipping on Enter.

#include "menus_listbox_internal.h"

#include "engine_offsets.h"
#include "engine_panels.h"
#include "log.h"
#include "menus_extract.h"
#include "menus_pending.h"
#include "speech.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// EquipPicker — modal item-pick zone inside the equipment screen.
// Armed when a slot Enter (BTN_INV_*) opens LB_ITEMS for that slot;
// disarmed on row commit, Esc, panel close, or panel-pointer drift.
// ============================================================================

bool EquipPickerMatchesPanel(void* p) {
    return IdentifyPanel(p) == PanelKind::InGameEquip;
}

bool EquipPickerArmed() { return IsEquipPickerArmed(); }

// No stale-reset callback: armed-ness is read off the engine's picker-open bit
// on whichever InGameEquip panel is in the stack, so a re-open (fresh panel
// allocation) answers correctly by construction. The old callback existed to
// catch a mirrored flag drifting against a dead panel pointer, and there is no
// longer a mirror to drift.

void* EquipPickerFindLb(void* p) {
    // Engine member, not .gui id — variant equip_p.gui files renumber
    // LB_ITEMS (userlogs/077noequipment; docs/gui-id-audit.md).
    return acc::menus::detail::EquipPanelItemsListBox(p);
}

// Inline announce only on no-op clamp; normal moves are caught by
// MonitorEquipPickerSelection (menus_listbox_picker.cpp). NB: rowCount-1
// (template at row 0 excluded
// from user-visible totals) and r.newSel directly (not +1) since minSel=1
// already shifts indices to the 1-based user view.
void EquipPickerAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (r.newSel != r.oldSel) return;
    if (!r.row) return;
    char rowText[256];
    if (acc::menus::extract::FromControl(r.row, rowText, sizeof(rowText))) {
        char msg[320];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
                 rowText, r.newSel, r.rowCount - 1);
        acc::speech::Speak(msg, /*interrupt=*/false);
    }
}

void EquipPickerLogExtra(char* out, size_t outN, const ListBoxNavResult& r) {
    snprintf(out, outN, ", real=%d", r.rowCount - 1);
}

// Custom Enter: lookup row + btn from listbox.selection_index, queue an
// EquipCommit op, disarm. Direct call to OnItemSelected/OnOKPressed
// bypasses click-sim entirely — see docs/equip-flow-investigation.md.
bool EquipPickerOnEnter(void* panel) {
    if (acc::menus::pending::IsPending()) {
        acclog::Write("EquipPicker", "Enter — op already pending; ignoring");
        ClearEquipPickerArmLatch();
        return true;
    }
    void* lb  = acc::menus::detail::EquipPanelItemsListBox(panel);
    void* btn = acc::menus::detail::EquipPanelEquipButton(panel);
    void* row       = nullptr;  // the user-selected row (an equippable item)
    void* unequipRow = nullptr; // row 0 — the always-present 0x7f000000 entry
    short selIdx = -1;
    int   rowCount = 0;
    if (lb) {
        auto* lbBase = reinterpret_cast<unsigned char*>(lb);
        auto* lbList = reinterpret_cast<CExoArrayList*>(
            lbBase + kListBoxControlsOffset);
        rowCount = (lbList && lbList->data) ? lbList->size : 0;
        selIdx = *reinterpret_cast<short*>(
            lbBase + kListBoxSelectionIndexOffset);
        if (lbList && lbList->data) {
            if (selIdx >= 1 && selIdx < rowCount)
                row = lbList->data[selIdx];
            // OnEnterSlot always inserts the "empty" entry (item id
            // 0x7f000000) at row 0; it's hidden from our nav (minSel=1) but
            // committing it drives the engine's unequip path.
            if (rowCount >= 1)
                unequipRow = lbList->data[0];
        }
    }

    // Is the selected row the item currently equipped in this slot? OnEnterSlot
    // tags that row with field6_0x394 bit 0x2 (the same flag that appends
    // " (Ausgew.)"). If so, Enter means "take it off" — re-equipping the same
    // item is a no-op in the engine, so we route the commit to the row-0 empty
    // entry, whose 0x7f000000 id sends OnItemSelected into its UnequipItem
    // branch. See docs/equip-flow-investigation.md.
    bool selectedIsEquipped = false;
    if (row) {
        uint32_t flags = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(row) + kEquipItemEntryFlagsOffset);
        selectedIsEquipped = (flags & kEquipItemEntryEquippedBit) != 0;
    }

    // Gates OnItemSelected reads are satisfied for either committed entry:
    // the queue's drain raises the entry's is_active; description_listbox.
    // bit_flags & 2 was raised by OnSelectSlot's ShowDescription;
    // items_listbox.bit_flags & 8 by OnSelectSlot's SetEnabled.
    if (selectedIsEquipped && unequipRow && btn) {
        acc::menus::pending::QueueEquipCommit(panel, unequipRow, btn);
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::EquipUnequipped),
                     /*interrupt=*/false);
        acclog::Write("EquipPicker", "Enter -> unequip (equipped row sel=%d, "
                      "commit empty row %p btn_equip=%p panel=%p)",
                      selIdx, unequipRow, btn, panel);
    } else if (lb && row && btn) {
        acc::menus::pending::QueueEquipCommit(panel, row, btn);
        acclog::Write("EquipPicker", "Enter -> commit (row sel=%d %p "
                      "btn_equip=%p panel=%p)",
                      selIdx, row, btn, panel);
    } else {
        acclog::Write("EquipPicker", "Enter -- can't equip "
                      "(lb=%p row=%p btn=%p sel=%d rows=%d) panel=%p",
                      lb, row, btn, selIdx, rowCount, panel);
    }
    ClearEquipPickerArmLatch();
    return true;
}

// Custom Esc: run the ENGINE's cancel, then drop our latch.
//
// Dropping the latch alone is not backing out. Arrowing the list equips as it
// goes (OnItemSelected is the row's select handler and calls EquipItem
// directly), and the engine's picker-open bit stays raised until something
// reaches CloseDescription. So a latch-only Esc left the previewed item
// permanently worn AND left ShowDescription(1)'s work in place — the interactive
// bit cleared on all nine slot buttons and labels — which is why every slot read
// "unavailable" from the first picker use until the screen was closed and
// reopened (K2 patch-20260817-065149.log lines 3110 onwards).
//
// QueueEquipPickerCancel dispatches the panel's own HandleInputEvent with the
// engine code Escape maps to, which restores the pre-browse item (and the
// off-hand a two-hander displaced), releases the remembered copies and calls
// CloseDescription. Same shape as WorkbenchUpgradeOnEsc below, which already
// had to learn this lesson on the mod picker.
//
// The engine clears its own bit inside that call, so the latch is all we clear
// here; IsEquipPickerArmed keeps answering true until the queued op drains,
// which is correct — the slots are still disabled for that tick.
bool EquipPickerOnEsc(void* panel) {
    if (!acc::menus::pending::IsPending()) {
        acc::menus::pending::QueueEquipPickerCancel(panel);
    }
    acclog::Write("EquipPicker", "Esc -> engine cancel + clear latch (panel=%p)",
                  panel);
    ClearEquipPickerArmLatch();
    return true;
}

}  // namespace

constexpr ListBoxPanelSpec kEquipPickerSpec = {
    /*logTag*/                  "EquipPicker",
    /*matches*/                 EquipPickerMatchesPanel,
    /*armed*/                   EquipPickerArmed,
    /*resetStale*/              nullptr,  // engine bit can't go stale
    /*findListBox*/             EquipPickerFindLb,
    /*minSel*/                  1,  // skip protoitem template at row 0
    /*announce*/                EquipPickerAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                EquipPickerLogExtra,
    /*onEnter*/                 EquipPickerOnEnter,
    /*onEsc*/                   EquipPickerOnEsc,
    /*titleOverride*/           nullptr,
    /*emptyStateId*/            acc::strings::Id::Count_,
    /*alwaysReturnFromHandler*/ false,  // fall through so slot-zone nav still works
    /*minSelFn*/                nullptr,
    /*useEngineSelect*/         true,  // engine SetSelectedControl + cursor parked off-list
};

}  // namespace acc::menus::listbox
