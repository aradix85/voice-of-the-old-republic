// Enter on the chain cursor: the per-panel activation routes and the generic activate.

#include "menus_chain.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_player.h"   // ReadPartyPortrait — party-full Enter check
#include "engine_reads.h"
#include "log.h"
#include "menus_crafting.h"
#include "menus_internal.h"
#include "menus_journal.h"
#include "menus_listbox.h"
#include "menus_modsettings.h"
#include "menus_monitors.h"
#include "menus_pending.h"
#include "menus_store.h"
#include "minigame_pazaak.h"  // kWagerLessCode / kWagerMoreCode
#include "peek_description.h"
#include "speech.h"
#include "strings.h"

#include <windows.h>

#include <cstdint>

namespace acc::menus::chain {

// Defined in menus_chain.cpp next to the chain-build code. Not in
// menus_chain.h because it is internal to the chain module - the Enter
// handler below is its only caller outside that file.
void SpeakLevelUpDoStepFirst();

void HandleEnterActivation(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputEnter1 && code != kInputEnter2) return;
    if (activePanel == nullptr) return;

    // Lazy rebind: previously Enter required a prior arrow press, which
    // stranded engine-pushed modals (StatusSummary after a skill check, the
    // quit-confirm MessageBox, AreaTransition prompts) where the chain was
    // bound to the previous panel. Mirroring HandleNavStep's rebind here lets
    // popups the engine pre-focuses (quit-confirm pre-focused on Cancel)
    // activate the focused button on Enter alone. RebindChain anchors
    // g_chainIndex on panel.activeControl when present.
    if (g_chainPanel != activePanel) {
        RebindChain(activePanel);
    }
    if (g_chainPanel != activePanel) return;          // rebind landed elsewhere
    if (g_chainCount <= 0) return;
    if (g_chainIndex < 0 || g_chainIndex >= g_chainCount) return;

    ChainEntry& e = g_chain[g_chainIndex];

    // Virtual chain entries route through their owning module BEFORE any
    // subclass-specific reads below — the entry's `control` field is a
    // sentinel pointer and any vtable / offset dereference would AV.
    // Currently only the mod-settings root entry; new virtual kinds add a
    // case here.
    if (e.virtualKind == kVirtualMod_SettingsRoot) {
        acc::menus::modsettings::OpenSubMenu(g_chainPanel);
        acclog::Write("Menus.Enter", "open mod-settings submenu (parent=%p)",
                      g_chainPanel);
        consumed = true;
        return;
    }

    bool isTabButton = false;
    if (g_tabbedPanel && g_tabsCount >= 2) {
        auto* tlist = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(g_tabbedPanel) + kPanelControlsOffset);
        if (tlist && tlist->data) {
            for (int i = g_tabsStart;
                 i < g_tabsStart + g_tabsCount && i < tlist->size; ++i) {
                if (tlist->data[i] == e.control) { isTabButton = true; break; }
            }
        }
    }

    // Detect equip-screen slot buttons up front. They need the full click
    // pipeline (cursor warp + LMouseDown/Up) to fire the engine's OnSelectSlot
    // — which is what populates LB_ITEMS with items matching the slot. Direct
    // vtable[15] activate on a slot button routes to a different handler
    // (likely OnEnterSlot, the keyboard shortcut path) that pops a "no items"
    // modal instead of populating the picker. Same gate-mismatch shape as
    // Options tab buttons: the mouse path is the only one that triggers the
    // populate.
    // Matched by position in the engine's embedded slot-button array, not
    // by .gui id — variant equip_p.gui files renumber the ids
    // (userlogs/077noequipment; docs/gui-id-audit.md).
    bool isEquipSlot = false;
    int  equipSlotIdx = -1;
    if (acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::InGameEquip) {
        equipSlotIdx = acc::menus::detail::EquipSlotIndexFromButton(
            g_chainPanel, e.control);
        isEquipSlot = equipSlotIdx >= 0;
    }

    // Workbench upgrade slot buttons (matched by position in the panel's
    // embedded button run — UpgradeSlotIndexFromButton; variant .gui files
    // renumber ids, docs/gui-id-audit.md). Same shape as equip-screen slot
    // buttons: direct vtable[15] activate doesn't populate LB_ITEMS with the
    // mods compatible with this slot — only the mouse-driven hover+click
    // pipeline does. We don't have an RE'd equivalent of
    // OnEnterSlot/OnSelectSlot for the workbench yet, so the safe path is a
    // full click-sim at the chain entry's extent center (mirrors the
    // tab-button activation pattern).
    bool isWorkbenchUpgradeSlot = false;
    int  workbenchUpgradeSlotIdx = -1;
    if (acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::WorkbenchUpgrade) {
        workbenchUpgradeSlotIdx =
            acc::menus::detail::UpgradeSlotIndexFromButton(g_chainPanel,
                                                           e.control);
        isWorkbenchUpgradeSlot = workbenchUpgradeSlotIdx >= 0;
    }

    // Store item row Enter — route to the engine's trade-action handler
    // (OnControlInvAButton / OnControlStoreAButton based on mode) instead of
    // the generic FireActivate. The default vtable[15] event 0x27 path just
    // refreshes the description listbox via OnControlEntered — never actually
    // sells or buys. Action buttons (Sell / Close / Buy) fall
    // through to the default activate path below; they're plain CSWGuiButton
    // instances, not CSWGuiStoreItemEntry rows.
    bool isStoreItemRow =
        acc::menus::store::IsStorePanel(g_chainPanel) &&
        acc::menus::store::IsStoreItemRow(e.control);

    // K2 workbench / crafting list-row Enter — "select this row, then fire
    // the panel's commit button" as one deferred op. Covers the K2
    // workbench top screen's LB_UPGRADELIST (commit = BTN_UPGRADEITEMS,
    // opens the upgrade flow for the row's item) and the create/breakdown
    // screens' visible item list (commit = BTN_Accept). The generic
    // vtable[15] activate on these rows only refreshes the description —
    // same shape as store rows. No-op resolve on K1 and on every other
    // panel kind.
    bool  isCraftRow = false;
    void* craftRowListBox = nullptr;
    if (!isStoreItemRow) {
        isCraftRow = acc::menus::crafting::ResolveRowCommit(
            g_chainPanel, e.control, &craftRowListBox);
    }

    // Journal quest-row Enter — read the description text. The row's own
    // activate handler is a no-op in the engine; the description text the
    // engine paints next to the list on mouse hover is the only signal a
    // sighted user gets, so we surface it on Enter instead.
    bool isJournalRow =
        acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::InGameJournal &&
        acc::menus::journal::IsJournalEntry(e.control);

    // Quest-item row Enter — the QuestItem sub-screen ("Quest Items")
    // lists plot items as CSWGuiInGameItemEntry rows with no meaningful
    // activate action (only BTN_BACK does anything). Mirror the journal: read
    // the item's property description on Enter. BTN_BACK is a plain button
    // (not an item row) so it falls through to the generic activate → close.
    bool isQuestItemRow =
        acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::InGameQuestItems &&
        acc::engine::IsInventoryItemRow(e.control);

    // PartySelection portrait Enter where the add will be refused because the
    // party is full. The engine's click handler (K1 OnToggled @0x006bf2a0,
    // K2 0x0089EB50) early-outs an *add* (portrait not yet selected) once two
    // companions are chosen — the panel's chosen count climbs to 2 and the
    // toggle does nothing, leaving no state change for the focus monitor to
    // re-announce. We still let the engine run (the no-op path plays its UI
    // click sound and refreshes the count label) but speak "Party full" so
    // the silent refusal is audible. Not for a story-locked companion: the
    // engine refuses that one for its own reason (KOTOR 2 says why in a
    // message box), and the portrait already announced the lock.
    bool isPartyAddBlocked = false;
    if (acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::PartySelection) {
        acc::engine::PartyPortrait portrait;
        if (acc::engine::ReadPartyPortrait(e.control, &portrait)) {
            isPartyAddBlocked =
                portrait.selected == 0 &&
                acc::engine::PartyTableIsNPCSelectable(portrait.npcSlot) &&
                acc::engine::PartySelectionChosenCount(g_chainPanel) > 1;
        }
    }

    // InGameLevelUp category / Accept the engine has not currently enabled.
    // The wizard enforces sequential leveling: only the "current" step carries
    // bit_flags bit 3 (0x8 = CSWGuiControl::SetEnabled); a real mouse click is
    // inert on the rest. Our deferred FireActivate force-raises is_active and
    // would otherwise let the user open a later step out of order — and doing
    // the Powers (powers) step before an earlier one makes the engine's
    // ChangeState/ClearPowers resync wipe the freshly-chosen power on commit
    // (root cause: memory project_levelup_power_lost_investigation). Block the
    // activation and point the user at the step the engine wants next. bit 3
    // is the real gate (bit 1 / is_active were both wrong on different button
    // sets); Accept gains bit 3 only once every step is done, so this also
    // correctly refuses a premature accept without trapping the user.
    bool isLevelUpInactiveStep = false;
    if (acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::InGameLevelUp) {
        __try {
            uint32_t bf = *reinterpret_cast<uint32_t*>(
                reinterpret_cast<unsigned char*>(e.control) +
                kControlBitFlagsOffset);
            isLevelUpInactiveStep = (bf & 0x8) == 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            isLevelUpInactiveStep = false;
        }
    }

    // Pazaak wager popup less/more buttons (CSWGuiSpeedButton).
    // They act only on their push callback (OnMinus/OnPlusButtonPushed ->
    // panel HandleInputEvent 0x2f/0x30); the generic vtable[15] activate
    // (0x27) they ignore — its switch case 0x27 is the popup's *commit*, and
    // the button-level activate never reaches it, so Enter was a silent no-op.
    // The wager also starts AT the maximum (the constructor seeds current =
    // max), so "more" is a no-op until "less" has lowered it. Route Enter to
    // the panel's own less/more dispatch. Which button is which comes from
    // the popup's ctor-bound members (WagerPopupPanel*), not from the .gui
    // ids the two games disagree on.
    bool isWagerStepButton = false;
    int  wagerStepCode = 0;
    if (acc::engine::IdentifyPanel(g_chainPanel) ==
            acc::engine::PanelKind::PazaakWager) {
        if (e.control ==
            acc::menus::detail::WagerPopupPanelLessButton(g_chainPanel)) {
            isWagerStepButton = true;
            wagerStepCode = acc::pazaak::kWagerLessCode;
        } else if (e.control ==
                   acc::menus::detail::WagerPopupPanelMoreButton(g_chainPanel)) {
            isWagerStepButton = true;
            wagerStepCode = acc::pazaak::kWagerMoreCode;
        }
    }

    if (acc::menus::pending::IsPending()) {
        acclog::Write("Enter", "op already pending; ignoring (target=%p)",
                      e.control);
        consumed = true;
    } else if (isLevelUpInactiveStep) {
        SpeakLevelUpDoStepFirst();
        acclog::Write("Menus.Enter",
                      "level-up step not enabled (bit3 clear) — blocked "
                      "out-of-order activation panel=%p index=%d target=%p",
                      g_chainPanel, g_chainIndex, e.control);
        consumed = true;
    } else if (isStoreItemRow) {
        acc::menus::pending::QueueStoreItemActivate(g_chainPanel, e.control);
        acclog::Write("Menus.Enter",
                      "store-item-activate panel=%p index=%d target=%p",
                      g_chainPanel, g_chainIndex, e.control);
        consumed = true;
    } else if (isCraftRow) {
        acc::menus::pending::QueueCraftRowCommit(
            g_chainPanel, craftRowListBox, e.control);
        acclog::Write("Menus.Enter",
                      "craft-row-commit panel=%p index=%d target=%p lb=%p",
                      g_chainPanel, g_chainIndex, e.control,
                      craftRowListBox);
        consumed = true;
    } else if (isJournalRow) {
        acc::menus::journal::SpeakDescription(g_chainPanel, e.control);
        consumed = true;
    } else if (isQuestItemRow) {
        acc::peek::SpeakItemRowDescription(e.control);
        acclog::Write("Menus.Enter",
                      "quest-item-description panel=%p index=%d target=%p",
                      g_chainPanel, g_chainIndex, e.control);
        consumed = true;
    } else if (e.textOnly) {
        // Modal body text — non-activatable. Re-speak so a user who missed
        // the open-time announce can hear it again. Don't fire vtable[15]
        // (the listbox has no activate handler).
        acc::menus::monitors::AnnounceControl(e.control);
        acclog::Write("Menus.Enter",
                      "re-announce panel=%p index=%d target=%p (text-only)",
                      activePanel, g_chainIndex, e.control);
        consumed = true;
    } else if (isTabButton) {
        int cursorY = e.cy + ComputeTabClickOffsetY(g_chainPanel);
        acc::menus::pending::QueueClickAt(e.cx, cursorY, e.control);
        acclog::Write("Menus.Enter",
                      "click-sim panel=%p index=%d target=%p cursorY=%d (tab)",
                      activePanel, g_chainIndex, e.control, cursorY);
        consumed = true;
    } else if (isEquipSlot) {
        // Bypass click-sim entirely. Calling OnEnterSlot then OnSelectSlot
        // directly invokes the same engine path that mouse-driven hover+click
        // does, but without depending on hit-test landing on the slot button
        // (the labels cover the buttons in z-order — see
        // docs/equip-flow-investigation.md). Deferred to OnUpdate to stay
        // clear of mid-input-dispatch recursion.
        acc::menus::pending::QueueEquipSelect(g_chainPanel, e.control);
        // Arm the picker zone now: OnSelectSlot raises field33_0x4270 |= 1
        // and the user proceeds to LB_ITEMS browsing. Self-clears on panel
        // close, picker Esc, or BTN_EQUIP dispatch.
        acc::menus::listbox::ArmEquipPicker(g_chainPanel, e.control);
        acclog::Write("EquipPicker",
                      "armed via direct OnEnterSlot+OnSelectSlot "
                      "(Enter on slot index=%d btn=%p panel=%p)",
                      equipSlotIdx, e.control, g_chainPanel);
        consumed = true;
    } else if (isWorkbenchUpgradeSlot) {
        // Click-sim landed on a label (z-order trap); vtable[15] is the
        // keyboard-shortcut path that doesn't populate LB_ITEMS. Both verified
        // in patch-20260525-141557.log and -142247.log. RE'd the workbench
        // slot-pick chain in Lane's gzf — calling CSWGuiUpgrade::OnEnterSlot
        // + OnSlotSelected directly is the engine path that builds the
        // compatible-mods list from CSWPartyTable items + upgrades_2da /
        // upcrystals_2da and AddControls-replaces LB_ITEMS contents.
        acc::menus::pending::QueueWorkbenchSlotSelect(g_chainPanel, e.control);
        acc::menus::listbox::ArmWorkbenchUpgradePicker(g_chainPanel);
        acclog::Write("WorkbenchUpgrade",
                      "armed via direct OnEnterSlot+OnSlotSelected "
                      "(Enter on slot index=%d btn=%p panel=%p)",
                      workbenchUpgradeSlotIdx, e.control, g_chainPanel);
        consumed = true;
    } else if (isWagerStepButton) {
        acc::menus::pending::QueueWagerInput(g_chainPanel, wagerStepCode);
        acclog::Write("Menus.Enter",
                      "pazaak-wager-step panel=%p index=%d target=%p code=0x%x",
                      g_chainPanel, g_chainIndex, e.control, wagerStepCode);
        consumed = true;
    } else if (isPartyAddBlocked) {
        // Run the engine's (no-op) toggle for the UI click sound, then tell
        // the user why the composition didn't change.
        acc::menus::pending::QueueActivate(e.control);
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::PartySelectionFull),
                     /*interrupt=*/false);
        acclog::Write("Menus.Enter",
                      "party-full add blocked panel=%p index=%d target=%p",
                      g_chainPanel, g_chainIndex, e.control);
        consumed = true;
    } else {
        acc::menus::pending::QueueActivate(e.control);
        // Drill flag is armed centrally inside the OnSwitchToSWInGameGui
        // detour — every path that opens a sub-screen (strip-icon Enter,
        // vanilla M/I/J hotkeys) flows through that one function, so no
        // per-caller arm is needed here.
        acclog::Write("Menus.Enter", "activate panel=%p index=%d target=%p",
                      activePanel, g_chainIndex, e.control);
        consumed = true;
    }
}

}  // namespace acc::menus::chain
