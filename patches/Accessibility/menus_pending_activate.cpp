// Activating a control by hand (vtable HandleInputEvent), with the guards and repairs around it: a stale chain panel, a chargen sub-screen closing, a button that frees itself, and the journal and inventory lists the activation rebuilds.

#include "menus_pending_internal.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"   // IdentifyPanel / PanelKind — chargen sub-screen close
#include "log.h"
#include "engine_reads.h"    // LookupTlk for workbench slot-type strref
#include "menus.h"           // ClearPendingAnnounce — partner of InvalidateChain
#include "menus_chain.h"     // ValidateChainPanel — post-Activate stale-pointer guard
#include "menus_chargen_attr.h"     // IsChargenAttributesPanel — chargen sub-screen close
#include "menus_chargen_feats.h"    // IsChargenFeatsPanel — chargen sub-screen close
#include "menus_chargen_skills.h"   // IsChargenSkillsPanel — chargen sub-screen close
#include "menus_internal.h"  // PowersPanel*/FeatsPanel* Accept/Back — chargen sub-screen close
#include "menus_inventory.h"  // filter list-rebuild repair + item-row selection sync
#include "menus_journal.h"   // Sort/Swap post-activate list-rebuild repair
#include "menus_powers_levelup.h"   // IsPowersLevelUpPanel — chargen sub-screen close

namespace acc::menus::pending {

// Direct activate via vtable[15].HandleInputEvent(0x27, 1). Bypasses
// hit-test, so a button covered by a listbox extent (e.g. Close
// in an Options sub-dialog) still fires its onClick when we target it.
//
// Raise is_active=1 before the dispatch to mirror what HandleLMouseDown
// does in the engine's mouse pipeline. Several engine onClick handlers
// gate on `this->is_active != 0` (memory: oncontrolentered_is_active_gate)
// and silently divert to a different code path when the flag is zero.
// For MessageBox OK buttons that divergent path appeared to corrupt
// engine state during close — three Windows __fastfail dumps today all
// landed between this FireActivate and the next frame, with the same
// MessageBox foreground each time (quit-confirm, save-overwrite,
// tutorial-hint — all the same engine MessageBox singleton). Forcing
// is_active=1 keeps the dispatch on the same code path the mouse
// pipeline takes.
//
// ONLY raise from 0 → 1: HandleLMouseDown's actual behaviour is a
// conditional raise, not an unconditional write. Some controls (tab
// buttons in InGameOptions, equip-slot buttons in InGameEquip) carry
// non-zero engine-bookkeeping values in this field (selected-tab
// marker, equip-state flags). Clobbering those with 1 corrupts the
// engine's view of which tab is selected, which surfaces later as a
// crash in unrelated subsystems on the next render/physics tick. A
// session FireActivate audit across 9 events found 5→1 (Feedback tab)
// was the only non-trivial clobber across all of testing, and the only
// session it appeared in is the one that crashed in CSWRoomSurfaceMesh
// AABB traversal moments after a tab-subdialog close.
//
// Post-activation re-announce is handled generically by
// MonitorFocusedControl on the next tick: toggles flip +0x1c8 bit 0
// synchronously inside FireActivate, cycles rewrite the value-display
// button's CExoString in place, sliders mutate cur_value at +0x74. All
// three produce a different ExtractAnnounceableText on next entry, and
// the monitor speaks the diff.
void DrainActivate(const PendingOp& op) {
    if (op.a) {
        // Snapshot the panel kind before dispatch — used post-fn to
        // decide whether op.a may have freed itself. Done up front
        // because the LevelUp Accept path is exactly when we
        // *can't* re-identify the panel safely after fn() returns.
        acc::engine::PanelKind panelKindAtDispatch =
            acc::engine::IdentifyPanel(acc::menus::chain::g_chainPanel);

        // Capture chargen sub-screen close intent before the
        // dispatch — the sub-screen panel may be freed inside fn()
        // when Accept/Cancel close it, so reading its vtable
        // afterwards is unsafe. Sub-screens of InGameLevelUp share
        // a .gui-id convention: BTN_ACCEPT=11 ("OK" / "Accept"),
        // BTN_BACK=12 ("Cancel"). Closing one of these via id
        // 11/12 is the same shape as the InGameOptions sub-screen
        // Esc close that already InvalidateChain's in menus.cpp —
        // without it, the parent InGameLevelUp panel's updated
        // chain entries ("Attribute, unavailable" etc.) can
        // stay silent on restore because channel-0 dedup and the
        // monitor's last-text cache still hold pre-close text.
        bool chargenSubClosing = false;
        if (panelKindAtDispatch != acc::engine::PanelKind::InGameLevelUp) {
            void* chainPanel = acc::menus::chain::g_chainPanel;
            if (acc::menus::powers_levelup::IsPowersLevelUpPanel(
                    chainPanel)) {
                // Engine-member compare (gui-id audit): the powers
                // panel's Accept/Back are ctor-bound embedded members,
                // so pointer equality is exact — no id convention, no
                // text disambiguation. This also fixes KOTOR 2, where
                // the old id 11/12 probe missed entirely (K2 numbers
                // Accept=10/Back=9; 11 is RECOMMENDED_BTN).
                chargenSubClosing =
                    op.a == acc::menus::detail::PowersPanelAcceptButton(
                                chainPanel) ||
                    op.a == acc::menus::detail::PowersPanelBackButton(
                                chainPanel);
            } else if (acc::menus::chargen_feats::IsChargenFeatsPanel(
                           chainPanel)) {
                // Engine-member compare (gui-id audit), same as the
                // powers panel above: the feats panel's Accept/Back
                // are ctor-bound embedded members, so pointer
                // equality is exact. The id 11/12 probe was doubly
                // wrong on KOTOR 2 — it numbers Accept=10/Back=9,
                // and its id 12 is LB_FEATS, a listbox.
                chargenSubClosing =
                    op.a == acc::menus::detail::FeatsPanelAcceptButton(
                                chainPanel) ||
                    op.a == acc::menus::detail::FeatsPanelBackButton(
                                chainPanel);
            } else if (acc::menus::chargen_attr::IsChargenAttributesPanel(
                           chainPanel) ||
                       acc::menus::chargen_skills::IsChargenSkillsPanel(
                           chainPanel)) {
                // Still id-based (11/12 = Accept/Cancel by the
                // InGameLevelUp sub-screen convention) until those
                // surfaces get their audit batch.
                int btnId = *reinterpret_cast<int*>(
                    reinterpret_cast<unsigned char*>(op.a) + kControlIdOffset);
                if (btnId == 11 || btnId == 12) {
                    // Disambiguate Accept/Cancel from same-id
                    // sibling controls (e.g. chargen-skills row arrows
                    // sometimes get id 11/12 too — Demolitions' right
                    // arrow has id 11). Accept/Cancel are labeled
                    // buttons; arrows have empty rendered text.
                    char text[32];
                    bool hasText = acc::engine::ReadButtonText(
                        op.a, text, sizeof(text)) && text[0] != '\0';
                    if (hasText) {
                        chargenSubClosing = true;
                    }
                }
            }
        }

        // Inventory item rows: point the listbox selection at the row we
        // are about to activate. Its handler reads the selection instead
        // of the control it was passed, so without this every Enter uses
        // row 0's item — see inventory::SyncListBoxSelection. Done here
        // rather than at queue time so nothing can move the selection in
        // the window before dispatch.
        if (panelKindAtDispatch == acc::engine::PanelKind::InGameInventory &&
            acc::engine::IsInventoryItemRow(op.a)) {
            acc::menus::inventory::SyncListBoxSelection(
                acc::menus::chain::g_chainPanel, op.a);
        }

        // Never dispatch into a panel whose identity no longer holds:
        // if the chain's panel address has been reused by a different
        // panel class, op.a describes the OLD object's layout and the
        // handler we are about to call lives in freed memory. That is
        // the KOTOR 2 chargen crash (patch-20260819-221733: the
        // Abilities panel's buttons dispatched with is_active reading
        // 2622030025, and the Skills panel that took over the block
        // then died in the engine's own handler).
        if (!acc::menus::chain::ChainPanelIdentityHolds()) {
            acclog::Write("Update",
                          "FireActivate SKIPPED target=%p — chain panel "
                          "identity broken (stale/reused address)", op.a);
            acc::menus::chain::InvalidateChain();
            return;
        }

        uint32_t prevIsActive = RaiseIsActiveIfZero(op.a);
        acclog::Write("Update", "FireActivate target=%p is_active=%u%s",
                      op.a, prevIsActive,
                      prevIsActive == 0 ? "->1" : " (preserved)");
        void** vtable = *reinterpret_cast<void***>(op.a);
        if (vtable) {
            auto fn = reinterpret_cast<PFN_ControlHandleInputEvent>(
                vtable[kVtableHandleInputEvent]);
            if (fn) {
                acclog::Write("Update", "FireActivate dispatch target=%p vtable=%p fn=%p",
                              op.a, vtable, fn);
                fn(op.a, kInputActivate, 1);
                acclog::Write("Update", "FireActivate returned target=%p", op.a);
            }
        }
        // Self-destroying-button hazard: some buttons free themselves
        // inside the onClick we just dispatched. Accept on
        // InGameLevelUp is the known case (commits the level-up AND
        // pops the panel — the button's own memory is freed before
        // fn returns). Within the same tick the engine defers both
        // the panels[] pop and the controls[] removal, and the freed
        // memory still reads as the same engine vtable bytes, so
        // panel-presence, controls[]-membership, and vtable-range
        // checks all see a healthy-looking state. The next tick's
        // MonitorFocusedControl would then dereference the freed
        // pointer in g_chain and FromControl's internal SEH-caught
        // AV would interact with /GS to fastfail the process.
        //
        // The guard must be scoped tightly: nulling the chain entry
        // unconditionally is wrong for toggles, sliders, cycles, and
        // tabs — those mutate state in place, the engine does NOT
        // refocus afterward, and the chain entry needs to survive so
        // the next-tick monitor speaks the new value ("on"/"off",
        // updated slider %, new cycle text). Nulling them silently
        // drops the post-toggle announce.
        //
        // Scope: only the InGameLevelUp panel kind, which is the only
        // panel known to free a control synchronously inside Activate.
        // If a future panel kind exhibits the same teardown shape,
        // add it to this gate; don't widen unconditionally.
        if (panelKindAtDispatch == acc::engine::PanelKind::InGameLevelUp) {
            int clearedIdx = -1;
            for (int i = 0; i < acc::menus::chain::g_chainCount; ++i) {
                if (acc::menus::chain::g_chain[i].control == op.a) {
                    acc::menus::chain::g_chain[i].control = nullptr;
                    clearedIdx = i;
                    break;
                }
            }
            if (clearedIdx >= 0) {
                acclog::Write("Update",
                              "FireActivate post: chain[%d].control=%p "
                              "nulled (self-destroy-risk, panel=InGameLevelUp)",
                              clearedIdx, op.a);
            }
        }
        // Chargen sub-screen close — drop the whole chain so the
        // parent InGameLevelUp's chain restore is treated as a fresh
        // binding. Same mechanism the InGameOptions sub-screen Esc
        // path uses (menus.cpp escIsOptionsSub branch). ClearPendingAnnounce
        // partners with InvalidateChain to also flush any in-flight
        // SetActive echo that would carry pre-close text past the
        // chain rebuild.
        if (chargenSubClosing) {
            acc::menus::chain::InvalidateChain();
            acc::menus::ClearPendingAnnounce();
            acclog::Write("Update",
                          "FireActivate post: chain invalidated "
                          "(chargen sub-screen close, target=%p)",
                          op.a);
        }
        // Journal Sort/Swap repair. Both buttons mutate the quest list,
        // which leaves our cached chain pointing at the old (Sort: about
        // to be freed and rebuilt) rows. Sort rebuilds lazily in Draw()
        // next frame, so force a synchronous PopulateItemListBox first or
        // the chain rebuild would capture half-constructed rows (base
        // CSWGuiObject vtable, "control N" text — patch-20260603-090028.log).
        // Swap already repopulated synchronously inside its handler, so
        // just invalidate the chain to re-bind to the new list. The new
        // title is announced separately by the ContentChange monitor.
        if (panelKindAtDispatch == acc::engine::PanelKind::InGameJournal) {
            if (acc::menus::journal::IsSortButton(
                    acc::menus::chain::g_chainPanel, op.a)) {
                acc::menus::journal::ForceRepopulate(
                    acc::menus::chain::g_chainPanel);
                acc::menus::chain::InvalidateChain();
                acclog::Write("Update",
                              "FireActivate post: journal Sort — "
                              "repopulated + chain invalidated");
            } else if (acc::menus::journal::IsSwapButton(
                           acc::menus::chain::g_chainPanel, op.a)) {
                acc::menus::chain::InvalidateChain();
                acclog::Write("Update",
                              "FireActivate post: journal Swap — "
                              "chain invalidated");
            }
        }
        // Inventory filter repair — the journal-Sort case exactly. The
        // "Show: …" button dispatches cmd 0x29, which only picks the
        // next filter and raises the panel's repopulate bit; the list is
        // rebuilt through CheckFilter LAZILY in Draw() next frame. The row
        // controls are pooled and reused, so nothing dangles, but the
        // chain keeps the pre-filter row count and row→item mapping
        // (verified in patch-20260731-083448.log: count stayed 209 while
        // the same pointers began speaking different items). Invalidating
        // alone would race — an engine focus echo can rebind before Draw
        // and capture the old list again — so force the repopulate first,
        // which also clears the pending bit so Draw won't rebuild twice.
        if (panelKindAtDispatch == acc::engine::PanelKind::InGameInventory &&
            acc::menus::inventory::IsFilterButton(
                acc::menus::chain::g_chainPanel, op.a)) {
            acc::menus::inventory::ForceRepopulate(
                acc::menus::chain::g_chainPanel);
            acc::menus::chain::InvalidateChain();
            acclog::Write("Update",
                          "FireActivate post: inventory filter — "
                          "repopulated + chain invalidated");
        }

        // Companion full-panel-pop guard: if the engine cleared the
        // panel from mgr.panels[] inside the dispatch (rarer — the
        // observed cases defer the pop), drop the whole chain.
        acc::menus::chain::ValidateChainPanel();
    } else {
        acclog::Write("Update", "FireActivate target=NULL (skipped)");
    }
}

}  // namespace acc::menus::pending
