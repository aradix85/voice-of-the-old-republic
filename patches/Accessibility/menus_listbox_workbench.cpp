// The workbench: the item list and the upgrade picker (empty rows, the
// KOTOR 2 'none' row, installing on Enter, the title).

#include "menus_listbox_internal.h"

#include "engine_game.h"     // IsKotor2 — K2 one-step upgrade install
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_extract.h"
#include "menus_pending.h"
#include "speech.h"

namespace acc::menus::listbox {

namespace {

// ============================================================================
// WorkbenchUpgrade — slot detail (upgrade.gui). 29 controls; the LB_ITEMS
// listbox at ID 0 holds compatible upgrade mods from the player's
// inventory for the currently selected slot. Enter on a row commits
// (stage + BTN_ASSEMBLE) via QueueWorkbenchUpgradeCommit; Esc disarms the
// picker so chain nav resumes on the slot buttons.
//
// Spec is armed only after the user activates one of the seven slot
// buttons (BTN_UPGRADE3X/4X at .gui IDs 12..18) — the click-sim wired in
// menus_chain_input_enter.cpp's chain Enter handler runs the engine's slot-select via
// MoveMouseToPosition + LMouseDown/Up, which populates LB_ITEMS with the
// inventory mods compatible with that slot. While not armed, the spec
// falls through (armed() returns false) so arrow keys flow into chain
// nav and the user can move between slot buttons / BTN_ASSEMBLE / BTN_BACK.
//
// Esc-to-close (BTN_BACK at ID 28) when the picker is not armed is
// handled by an explicit workbench branch in menus_chain_input_keys.cpp's Esc handler —
// FindCancelButton resolves to "Cancel" reliably for this panel.
// ============================================================================
// LB_ITEMS / BTN_BACK / BTN_ASSEMBLE resolve through the UpgradePanel*
// member resolvers in menus_internal.cpp (engine ctor bindings — variant
// .gui files renumber the ids, docs/gui-id-audit.md). Only the title
// label is still id-addressed: announce-only, so a variant install
// degrades to the generic title walk instead of breaking input.
constexpr int kWorkbenchUpgradeTitleId     = 25;  // LBL_TITLE ("Workbench")

// ============================================================================
// WorkbenchItems — per-category item picker (upgradeitems.gui).
// LB_ITEMS at ID 0 holds the player's upgradable weapons in the chosen
// category. Enter → BTN_UPGRADEITEM (ID 4, "Upgrade") commits the
// selection and opens the slot-detail panel. Esc → BTN_BACK (ID 5,
// "Close") closes back to upgradesel.gui.
// ============================================================================

bool WorkbenchItemsMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::WorkbenchItems;
}

void* WorkbenchItemsFindLb(void* p) {
    return WorkbenchItemsPanelItemsListBox(p);
}

// Speak the focused weapon row + position. No per-tick monitor watches
// this listbox so we speak on every step (including clamp).
void WorkbenchItemsAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (r.rowCount <= 0) return;
    SpeakRowAndPosition(r);
}

bool WorkbenchItemsOnEnter(void* panel) {
    QueueControlActivate(WorkbenchItemsPanelUpgradeButton(panel),
                         "WorkbenchItems: Enter -> BTN_UPGRADEITEM");
    return true;
}

bool WorkbenchItemsOnEsc(void* panel) {
    QueueControlActivate(WorkbenchItemsPanelBackButton(panel),
                         "WorkbenchItems: Esc -> BTN_BACK");
    return true;
}

bool WorkbenchUpgradeMatches(void* p) {
    return IdentifyPanel(p) == PanelKind::WorkbenchUpgrade;
}

bool WorkbenchUpgradeArmed() { return IsWorkbenchUpgradePickerArmed(); }

// No stale-reset callback, for the same reason the equip spec has none: armed-
// ness is the engine's picker-open bit on whichever WorkbenchUpgrade panel is
// in the stack, so a re-open (fresh panel allocation) answers correctly on its
// own. There is no mirrored pointer left to go stale against.

void* WorkbenchUpgradeFindLb(void* p) {
    // Engine member, not .gui id — variant .gui files renumber ids
    // (docs/gui-id-audit.md).
    return acc::menus::detail::UpgradePanelItemsListBox(p);
}

// LB_ITEMS rows are CSWGuiInventoryItemEntry-style — their text comes
// from the item resref's localised name. Same announce shape as the
// items picker.
// The engine's compatible-mods list carries a "none / remove" entry whose
// display text is a bare "-" (dash). Spoken with the position suffix it
// reads as ", 1 of 1" — heard as "1" — or, on readers that swallow a lone
// dash, as silence. Both were reported from the K2 workbench. Treat a blank
// or dash-only row name as the none entry and speak a real word instead.
// Also covers the rare row whose text doesn't extract at all (would have
// been a silent step otherwise).
bool IsDashOrBlankRowText(const char* s) {
    if (!s) return true;
    while (*s == ' ' || *s == '\t') ++s;
    if (*s == '\0') return true;
    if (*s == '-') {
        ++s;
        while (*s == ' ' || *s == '\t') ++s;
        return *s == '\0';
    }
    return false;
}

void WorkbenchUpgradeAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (!r.row || r.rowCount <= 0) return;
    char rowText[256];
    bool readable = acc::menus::extract::FromControl(r.row, rowText,
                                                     sizeof(rowText)) &&
                    !IsDashOrBlankRowText(rowText);
    if (!readable) {
        // Unreadable row: the engine's remove entry says "No item";
        // a real mod row we simply can't name gets a generic "Upgrade"
        // (never masked as the none entry). On K1 (no key field) a dash row
        // is the remove entry, so it maps to none there.
        acc::strings::Id sid =
            (acc::menus::detail::IsUpgradePickerNoneRow(r.row) || !acc::game::IsKotor2())
                ? acc::strings::Id::WorkbenchPickerNone
                : acc::strings::Id::WorkbenchPickerUnnamed;
        const char* sub = acc::strings::Get(sid);
        size_t n = strnlen(sub, sizeof(rowText) - 1);
        memcpy(rowText, sub, n);
        rowText[n] = '\0';
    }
    // The picker layout (hidden remove entry vs. all-rows-real) decides the
    // first navigable row, so the spoken position is 1-based over the visible
    // rows only. Same panel the picker is armed against.
    auto info = acc::engine::GetWorkbenchPickerInfo(WorkbenchUpgradePickerPanel());
    int minSel = info.valid ? info.minSel : 0;

    // Mark the row that's already installed in / set on the slot.
    char marked[320];
    if (info.valid && info.installedRow >= 0 && r.newSel == info.installedRow) {
        snprintf(marked, sizeof(marked), "%s, %s", rowText,
                 acc::strings::Get(acc::strings::Id::WorkbenchPickerInstalled));
    } else {
        snprintf(marked, sizeof(marked), "%s", rowText);
    }

    char msg[384];
    snprintf(msg, sizeof(msg),
             acc::strings::Get(acc::strings::Id::FmtContainerItemAt),
             marked, r.newSel - minSel + 1, r.rowCount - minSel);
    acc::speech::Speak(msg, /*interrupt=*/false);
}

// Dynamic minSel: hide the row-0 remove entry on power slots (minSel 1); show
// every row on the colour slot (minSel 0). See GetWorkbenchPickerInfo.
int WorkbenchUpgradeMinSel(void* panel) {
    auto info = acc::engine::GetWorkbenchPickerInfo(panel);
    return info.valid ? info.minSel : 0;
}

// Enter (armed): commit the currently-selected LB_ITEMS row via a single
// pending op that fires vtable[15] on the row (stage), then on
// BTN_ASSEMBLE (install). Mirrors EquipPickerOnEnter's row-then-commit
// shape; disarms the picker after queueing.
//
// Remove gesture (power slots only): Enter on the currently-installed crystal
// row routes the commit to the hidden row-0 entry (item id 0x7f000000), whose
// OnUpgradeSelected branch removes the mod and returns it to inventory — the
// same trick the equip picker uses to unequip. The colour slot has no remove
// entry, so it always installs the selected colour.
bool WorkbenchUpgradeOnEnter(void* panel) {
    if (acc::menus::pending::IsPending()) {
        acclog::Write("WorkbenchUpgrade", "Enter — op already pending; ignoring");
        ClearWorkbenchUpgradeArmLatch();
        return true;
    }
    void* lb  = acc::menus::detail::UpgradePanelItemsListBox(panel);
    void* btn = acc::menus::detail::UpgradePanelAssembleButton(panel);
    const int rowCount = acc::engine::ReadListBoxRowCount(lb);
    const int selIdx = acc::engine::ReadListBoxSelection(lb);
    void* const row = acc::engine::ReadListBoxRow(lb, selIdx);
    // Row 0: the 0x7f000000 remove entry (power slots).
    void* const removeRow = acc::engine::ReadListBoxRow(lb, 0);

    // KOTOR 2: the row's own activate handler installs or removes directly
    // and closes the picker — one engine call, no stage+assemble. The engine
    // decides install vs. remove from the row (the "-" none row removes; a
    // real row installs). Fire OnUpgradeSelected(panel, row) only; K1 keeps
    // the two-step commit below.
    if (acc::game::IsKotor2()) {
        if (row) {
            // The outcome (installed / removed / refused for an incompatible
            // slot) is announced from the drain, which compares the slot's
            // field35 before vs. after the engine call — an immediate "installed"
            // here would lie when the engine refuses an incompatible mod.
            acc::menus::pending::QueueWorkbenchUpgradeInstallK2(panel, row);
            acclog::Write("WorkbenchUpgrade", "K2 install/remove queued (row sel=%d "
                          "%p none=%d panel=%p)", selIdx, row,
                          acc::menus::detail::IsUpgradePickerNoneRow(row) ? 1 : 0, panel);
        } else {
            acclog::Write("WorkbenchUpgrade", "K2 Enter -- no row (lb=%p sel=%d "
                          "rows=%d) panel=%p", lb, selIdx, rowCount, panel);
        }
        ClearWorkbenchUpgradeArmLatch();
        return true;
    }

    auto info = acc::engine::GetWorkbenchPickerInfo(panel);
    bool removeGesture = info.valid && !info.isColorSlot &&
                         info.installedRow >= 0 && selIdx == info.installedRow;

    if (removeGesture && removeRow && btn) {
        acc::menus::pending::QueueWorkbenchUpgradeCommit(panel, removeRow, btn);
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::WorkbenchSlotRemoved),
                     /*interrupt=*/false);
        acclog::Write("WorkbenchUpgrade", "Enter -> remove (installed row sel=%d, "
                      "commit remove row %p btn_assemble=%p panel=%p)",
                      selIdx, removeRow, btn, panel);
    } else if (row && btn) {
        acc::menus::pending::QueueWorkbenchUpgradeCommit(panel, row, btn);
        acclog::Write("WorkbenchUpgrade", "Enter -> commit (row sel=%d %p "
                      "btn_assemble=%p panel=%p)",
                      selIdx, row, btn, panel);
    } else {
        acclog::Write("WorkbenchUpgrade", "Enter -- can't commit "
                      "(lb=%p row=%p btn=%p sel=%d rows=%d) panel=%p",
                      lb, row, btn, selIdx, rowCount, panel);
    }
    ClearWorkbenchUpgradeArmLatch();
    return true;
}

// Esc (armed): disarm only. Chain focus is unchanged so the next arrow
// press resumes slot-button navigation. Esc when NOT armed (i.e. user
// is on a slot button or BTN_ASSEMBLE/BTN_BACK) is handled by the
// workbench-specific block in menus_chain_input_keys.cpp's Esc handler.
bool WorkbenchUpgradeOnEsc(void* panel) {
    // Back out of the picker the way commit does: close the item zone so the
    // engine re-enables the sibling slot buttons (OnSlotSelected disabled them
    // and only undoes it on commit). Without this they keep reading
    // "unavailable" until the workbench is closed and reopened.
    if (!acc::menus::pending::IsPending()) {
        acc::menus::pending::QueueWorkbenchPickerCancel(panel);
    }
    acclog::Write("WorkbenchUpgrade", "Esc -> cancel picker + disarm (panel=%p)", panel);
    ClearWorkbenchUpgradeArmLatch();
    return true;
}

// The generic title-walk would land on the first leftover placeholder label
// (id 19 "Item Property" — a design-time string the engine repopulates per
// slot at runtime, alongside id 20 "Slot Name" / id 21 "Lightsaber Slot Name")
// instead of the real title. Read LBL_TITLE (id 25, "Workbench") directly so
// first-sight speaks the panel name. Returns nullptr to fall back to the
// generic walk if the title control can't be read.
const char* WorkbenchUpgradeTitleOverride(void* panel) {
    if (!panel) return nullptr;
    void* title = FindControlById(panel, kWorkbenchUpgradeTitleId);
    if (!title) return nullptr;
    static char buf[128];
    if (acc::menus::extract::FromControl(title, buf, sizeof(buf)) && buf[0]) {
        return buf;
    }
    return nullptr;
}

}  // namespace

constexpr ListBoxPanelSpec kWorkbenchItemsSpec = {
    /*logTag*/                  "WorkbenchItems",
    /*matches*/                 WorkbenchItemsMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             WorkbenchItemsFindLb,
    /*minSel*/                  0,
    /*announce*/                WorkbenchItemsAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                nullptr,
    /*onEnter*/                 WorkbenchItemsOnEnter,
    /*onEsc*/                   WorkbenchItemsOnEsc,
    /*titleOverride*/           nullptr,
    /*emptyStateId*/            acc::strings::Id::WorkbenchItemsEmpty,
    /*alwaysReturnFromHandler*/ false,  // fall through so chain nav reaches buttons
};

constexpr ListBoxPanelSpec kWorkbenchUpgradeSpec = {
    /*logTag*/                  "WorkbenchUpgrade",
    /*matches*/                 WorkbenchUpgradeMatches,
    /*armed*/                   WorkbenchUpgradeArmed,
    /*resetStale*/              nullptr,  // engine bit can't go stale
    /*findListBox*/             WorkbenchUpgradeFindLb,
    /*minSel*/                  0,
    /*announce*/                WorkbenchUpgradeAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                nullptr,
    /*onEnter*/                 WorkbenchUpgradeOnEnter,
    /*onEsc*/                   WorkbenchUpgradeOnEsc,
    /*titleOverride*/           WorkbenchUpgradeTitleOverride,
    /*emptyStateId*/            acc::strings::Id::WorkbenchUpgradesEmpty,
    /*alwaysReturnFromHandler*/ false,  // fall through so chain nav reaches the slot/assemble buttons
    /*minSelFn*/                WorkbenchUpgradeMinSel,  // hide row-0 remove entry on power slots
    /*useEngineSelect*/         true,  // engine SetSelectedControl + cursor parked off-list
};

}  // namespace acc::menus::listbox
