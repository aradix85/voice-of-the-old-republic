// Activating a control by hand (its HandleInputEvent), with the guards and
// repairs around it: a stale chain panel, a chargen sub-screen closing, a
// button that frees itself, and the journal and inventory lists the
// activation rebuilds.

#include "menus_pending_internal.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_reads.h"
#include "log.h"
#include "menus.h"
#include "menus_chain.h"
#include "menus_chargen_attr.h"
#include "menus_chargen_feats.h"
#include "menus_chargen_skills.h"
#include "menus_internal.h"
#include "menus_inventory.h"
#include "menus_journal.h"
#include "menus_powers_levelup.h"

namespace acc::menus::pending {

namespace {

using acc::engine::PanelKind;

// True when `target` is the Accept or Cancel button of a chargen sub-screen
// of InGameLevelUp, i.e. the activation closes that sub-screen. Decided
// before the dispatch, which may free the sub-screen. Closing one needs the
// same chain reset as the InGameOptions sub-screen Esc close in menus.cpp:
// without it, the parent InGameLevelUp panel's updated entries ("Attribute,
// unavailable" etc.) can stay silent on restore, because channel-0 dedup
// and the monitor's last-text cache still hold the pre-close text.
bool ClosesChargenSubScreen(void* target, PanelKind kindAtDispatch) {
    if (kindAtDispatch == PanelKind::InGameLevelUp) return false;
    void* chainPanel = acc::menus::chain::g_chainPanel;
    if (acc::menus::powers_levelup::IsPowersLevelUpPanel(chainPanel)) {
        // Accept/Back are ctor-bound embedded members, so pointer equality is
        // exact; an id probe misses on KOTOR 2, which numbers them
        // Accept=10/Back=9 (its 11 is RECOMMENDED_BTN).
        return target == acc::menus::detail::PowersPanelAcceptButton(chainPanel) ||
               target == acc::menus::detail::PowersPanelBackButton(chainPanel);
    }
    if (acc::menus::chargen_feats::IsChargenFeatsPanel(chainPanel)) {
        // As for the powers panel; by id it would be doubly wrong on KOTOR 2,
        // whose id 12 is LB_FEATS, a listbox.
        return target == acc::menus::detail::FeatsPanelAcceptButton(chainPanel) ||
               target == acc::menus::detail::FeatsPanelBackButton(chainPanel);
    }
    if (acc::menus::chargen_attr::IsChargenAttributesPanel(chainPanel) ||
        acc::menus::chargen_skills::IsChargenSkillsPanel(chainPanel)) {
        // Still by id: 11/12 = Accept/Cancel by the InGameLevelUp sub-screen
        // convention. Row arrows can share those ids (Demolitions' right
        // arrow has id 11); Accept/Cancel are the ones with a label.
        int btnId = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(target) + kControlIdOffset);
        if (btnId != 11 && btnId != 12) return false;
        char text[32];
        return acc::engine::ReadButtonText(target, text, sizeof(text)) && text[0] != '\0';
    }
    return false;
}

// Self-destroying-button hazard: Accept on InGameLevelUp commits the
// level-up AND pops the panel, freeing the button before the handler
// returns. Within the same tick the engine defers both the panels[] pop and
// the controls[] removal, and the freed memory still reads as the same
// engine vtable bytes, so panel-presence, controls[]-membership and
// vtable-range checks all see a healthy-looking state. The next tick's
// MonitorFocusedControl would then dereference the freed pointer in g_chain,
// and FromControl's internal SEH-caught AV would interact with /GS to
// fastfail the process. So the chain entry is nulled.
//
// Only for InGameLevelUp, the one panel known to free a control inside
// Activate: toggles, sliders, cycles and tabs mutate in place, the engine
// does not refocus, and their chain entry must survive so the next-tick
// monitor speaks the new value. Add a panel kind here only when it shows
// the same teardown; don't widen it unconditionally.
void ForgetSelfDestroyingButton(void* target) {
    for (int i = 0; i < acc::menus::chain::g_chainCount; ++i) {
        if (acc::menus::chain::g_chain[i].control == target) {
            acc::menus::chain::g_chain[i].control = nullptr;
            acclog::Write("Update",
                          "FireActivate post: chain[%d].control=%p "
                          "nulled (self-destroy-risk, panel=InGameLevelUp)",
                          i, target);
            return;
        }
    }
}

// Journal Sort/Swap repair. Both buttons mutate the quest list, which leaves
// our cached chain pointing at the old (Sort: about to be freed and rebuilt)
// rows. Sort rebuilds lazily in Draw() next frame, so force a synchronous
// PopulateItemListBox first, or the chain rebuild would capture
// half-constructed rows (base CSWGuiObject vtable, "control N" text). Swap
// already repopulated synchronously inside its handler, so just invalidate
// the chain to re-bind to the new list. The new title is announced
// separately by the ContentChange monitor.
void RepairJournalList(void* target) {
    void* panel = acc::menus::chain::g_chainPanel;
    if (acc::menus::journal::IsSortButton(panel, target)) {
        acc::menus::journal::ForceRepopulate(panel);
        acc::menus::chain::InvalidateChain();
        acclog::Write("Update",
                      "FireActivate post: journal Sort — repopulated + chain invalidated");
    } else if (acc::menus::journal::IsSwapButton(panel, target)) {
        acc::menus::chain::InvalidateChain();
        acclog::Write("Update", "FireActivate post: journal Swap — chain invalidated");
    }
}

// Inventory filter repair — the journal-Sort case exactly. The "Show: …"
// button dispatches cmd 0x29, which only picks the next filter and raises
// the panel's repopulate bit; the list is rebuilt through CheckFilter
// LAZILY in Draw() next frame. The row controls are pooled and reused, so
// nothing dangles, but the chain keeps the pre-filter row count and
// row→item mapping (the same pointers began speaking different items).
// Invalidating alone would race — an engine focus echo can rebind before
// Draw and capture the old list again — so force the repopulate first,
// which also clears the pending bit so Draw won't rebuild twice.
void RepairInventoryFilter(void* target) {
    void* panel = acc::menus::chain::g_chainPanel;
    if (!acc::menus::inventory::IsFilterButton(panel, target)) return;
    acc::menus::inventory::ForceRepopulate(panel);
    acc::menus::chain::InvalidateChain();
    acclog::Write("Update",
                  "FireActivate post: inventory filter — repopulated + chain invalidated");
}

}  // namespace

// Direct activate via HandleInputEvent(0x27, 1). Bypasses hit-test, so a
// button covered by a listbox extent (e.g. Close in an Options sub-dialog)
// still fires its onClick. is_active is raised first, as the engine's mouse
// pipeline does (see RaiseIsActiveIfZero).
//
// Post-activation re-announce is handled generically by
// MonitorFocusedControl on the next tick: toggles flip +0x1c8 bit 0
// synchronously inside FireActivate, cycles rewrite the value-display
// button's CExoString in place, sliders mutate cur_value at +0x74. All
// three produce a different ExtractAnnounceableText on next entry, and
// the monitor speaks the diff.
void DrainActivate(const PendingOp& op) {
    if (!op.a) {
        acclog::Write("Update", "FireActivate target=NULL (skipped)");
        return;
    }
    // The panel kind is read before the dispatch: after InGameLevelUp's
    // Accept the panel cannot safely be identified again.
    const PanelKind kindAtDispatch =
        acc::engine::IdentifyPanel(acc::menus::chain::g_chainPanel);
    const bool chargenSubClosing = ClosesChargenSubScreen(op.a, kindAtDispatch);

    // Inventory item rows: point the listbox selection at the row we are
    // about to activate. Its handler reads the selection instead of the
    // control it was passed, so without this every Enter uses row 0's item —
    // see inventory::SyncListBoxSelection. Done here rather than at queue
    // time so nothing can move the selection in the window before dispatch.
    if (kindAtDispatch == PanelKind::InGameInventory &&
        acc::engine::IsInventoryItemRow(op.a)) {
        acc::menus::inventory::SyncListBoxSelection(acc::menus::chain::g_chainPanel, op.a);
    }

    // Never dispatch into a panel whose identity no longer holds: if the
    // chain's panel address has been reused by a different panel class, op.a
    // describes the OLD object's layout and the handler we are about to call
    // lives in freed memory (the KOTOR 2 chargen crash: the Abilities
    // panel's buttons dispatched with a garbage is_active, and the Skills
    // panel that took over the block died in the engine's own handler).
    if (!acc::menus::chain::ChainPanelIdentityHolds()) {
        acclog::Write("Update",
                      "FireActivate SKIPPED target=%p — chain panel "
                      "identity broken (stale/reused address)", op.a);
        acc::menus::chain::InvalidateChain();
        return;
    }

    uint32_t prevIsActive = RaiseIsActiveIfZero(op.a);
    acclog::Write("Update", "FireActivate target=%p is_active=%u%s",
                  op.a, prevIsActive, RaiseNote(prevIsActive));
    if (auto fn = HandleInputEventOf(op.a)) {
        acclog::Write("Update", "FireActivate dispatch target=%p fn=%p", op.a, fn);
        fn(op.a, kInputActivate, 1);
        acclog::Write("Update", "FireActivate returned target=%p", op.a);
    }

    if (kindAtDispatch == PanelKind::InGameLevelUp) ForgetSelfDestroyingButton(op.a);
    // A closed chargen sub-screen: drop the whole chain so the parent
    // InGameLevelUp's chain restore is a fresh binding; ClearPendingAnnounce
    // also flushes an in-flight SetActive echo carrying pre-close text.
    if (chargenSubClosing) {
        acc::menus::chain::InvalidateChain();
        acc::menus::ClearPendingAnnounce();
        acclog::Write("Update",
                      "FireActivate post: chain invalidated "
                      "(chargen sub-screen close, target=%p)", op.a);
    }
    if (kindAtDispatch == PanelKind::InGameJournal) RepairJournalList(op.a);
    if (kindAtDispatch == PanelKind::InGameInventory) RepairInventoryFilter(op.a);

    // Companion full-panel-pop guard: if the engine cleared the panel from
    // mgr.panels[] inside the dispatch (rarer — the observed cases defer the
    // pop), drop the whole chain.
    acc::menus::chain::ValidateChainPanel();
}

}  // namespace acc::menus::pending
