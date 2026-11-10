// Enter on the chain cursor: the per-panel activation routes and the generic
// activate.

#include "menus_chain.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_player.h"
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
#include "minigame_pazaak.h"
#include "peek_description.h"
#include "speech.h"
#include "strings.h"

#include <cstdint>

namespace acc::menus::chain {

// Defined in menus_chain.cpp next to the chain-build code; Enter is its only
// caller outside that file.
void SpeakLevelUpDoStepFirst();

namespace {

using acc::engine::PanelKind;

// True when the route took the Enter press on the focused entry.
using EnterRoute = bool (*)(PanelKind kind, const ChainEntry& e);

// The level-up wizard enables only the current step (bit_flags bit 3, from
// CSWGuiControl::SetEnabled); a mouse click is inert on the others. Our
// activate force-raises is_active, so without this gate a later step opens out
// of order, and Powers done early has its choice wiped by the engine's resync
// on commit. Accept gains bit 3 once every step is done.
bool LevelUpStepDisabled(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::InGameLevelUp) return false;
    uint32_t flags = 0;
    if (!acc::engine::TryReadU32(e.control, kControlBitFlagsOffset, &flags)) return false;
    if ((flags & 0x8) != 0) return false;
    SpeakLevelUpDoStepFirst();
    acclog::Write("Menus.Enter",
                  "level-up step not enabled (bit3 clear) — blocked "
                  "out-of-order activation panel=%p index=%d target=%p",
                  g_chainPanel, g_chainIndex, e.control);
    return true;
}

// The generic activate on a store row only refreshes the description; the
// trade goes through OnControlInvAButton / OnControlStoreAButton.
bool StoreItemRow(PanelKind, const ChainEntry& e) {
    if (!acc::menus::store::IsStorePanel(g_chainPanel) ||
        !acc::menus::store::IsStoreItemRow(e.control)) {
        return false;
    }
    acc::menus::pending::QueueStoreItemActivate(g_chainPanel, e.control);
    acclog::Write("Menus.Enter", "store-item-activate panel=%p index=%d target=%p",
                  g_chainPanel, g_chainIndex, e.control);
    return true;
}

// K2 workbench and crafting list rows: select the row, then fire the panel's
// commit button (BTN_UPGRADEITEMS, BTN_Accept), as one deferred op.
bool CraftRow(PanelKind, const ChainEntry& e) {
    void* lb = nullptr;
    if (!acc::menus::crafting::ResolveRowCommit(g_chainPanel, e.control, &lb)) return false;
    acc::menus::pending::QueueCraftRowCommit(g_chainPanel, lb, e.control);
    acclog::Write("Menus.Enter", "craft-row-commit panel=%p index=%d target=%p lb=%p",
                  g_chainPanel, g_chainIndex, e.control, lb);
    return true;
}

// Journal rows have no activate of their own: Enter reads the description a
// sighted player sees on hover.
bool JournalRow(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::InGameJournal ||
        !acc::menus::journal::IsJournalEntry(e.control)) {
        return false;
    }
    acc::menus::journal::SpeakDescription(g_chainPanel, e.control);
    return true;
}

// Quest-item rows likewise read the item's description; BTN_BACK is no item
// row and takes the generic activate.
bool QuestItemRow(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::InGameQuestItems || !acc::engine::IsInventoryItemRow(e.control)) {
        return false;
    }
    acc::peek::SpeakItemRowDescription(e.control);
    acclog::Write("Menus.Enter", "quest-item-description panel=%p index=%d target=%p",
                  g_chainPanel, g_chainIndex, e.control);
    return true;
}

// Modal body text has nothing to activate: say it again.
bool TextOnly(PanelKind, const ChainEntry& e) {
    if (!e.textOnly) return false;
    acc::menus::monitors::AnnounceControl(e.control);
    acclog::Write("Menus.Enter", "re-announce panel=%p index=%d target=%p (text-only)",
                  g_chainPanel, g_chainIndex, e.control);
    return true;
}

// Tab buttons switch only through the mouse path.
bool TabButton(PanelKind, const ChainEntry& e) {
    if (!IsTabButton(e.control)) return false;
    const int cursorY = e.cy + ComputeTabClickOffsetY(g_chainPanel);
    acc::menus::pending::QueueClickAt(e.cx, cursorY, e.control);
    acclog::Write("Menus.Enter", "click-sim panel=%p index=%d target=%p cursorY=%d (tab)",
                  g_chainPanel, g_chainIndex, e.control, cursorY);
    return true;
}

// Equip slot buttons: the generic activate takes the keyboard-shortcut path,
// which pops "no items" instead of filling LB_ITEMS. OnEnterSlot plus
// OnSelectSlot, called directly, is what a mouse hover and click run; a
// click-sim would land on the labels above the buttons. Matched by position
// in the slot array, since variant .gui files renumber the ids.
bool EquipSlot(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::InGameEquip) return false;
    const int slot = acc::menus::detail::EquipSlotIndexFromButton(g_chainPanel, e.control);
    if (slot < 0) return false;
    acc::menus::pending::QueueEquipSelect(g_chainPanel, e.control);
    // The picker disarms on panel close, picker Esc or BTN_EQUIP.
    acc::menus::listbox::ArmEquipPicker(g_chainPanel, e.control);
    acclog::Write("EquipPicker",
                  "armed via direct OnEnterSlot+OnSelectSlot "
                  "(Enter on slot index=%d btn=%p panel=%p)",
                  slot, e.control, g_chainPanel);
    return true;
}

// Workbench upgrade slots, the same way: CSWGuiUpgrade::OnEnterSlot plus
// OnSlotSelected build the list of compatible mods into LB_ITEMS.
bool WorkbenchUpgradeSlot(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::WorkbenchUpgrade) return false;
    const int slot = acc::menus::detail::UpgradeSlotIndexFromButton(g_chainPanel, e.control);
    if (slot < 0) return false;
    acc::menus::pending::QueueWorkbenchSlotSelect(g_chainPanel, e.control);
    acc::menus::listbox::ArmWorkbenchUpgradePicker(g_chainPanel);
    acclog::Write("WorkbenchUpgrade",
                  "armed via direct OnEnterSlot+OnSlotSelected "
                  "(Enter on slot index=%d btn=%p panel=%p)",
                  slot, e.control, g_chainPanel);
    return true;
}

// The Pazaak wager popup's less/more buttons act only on their push callback
// (the panel's HandleInputEvent 0x2f/0x30), not on the generic activate. They
// are found through the popup's members, since the games' .gui ids differ.
bool WagerStepButton(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::PazaakWager) return false;
    int code = 0;
    if (e.control == acc::menus::detail::WagerPopupPanelLessButton(g_chainPanel)) {
        code = acc::pazaak::kWagerLessCode;
    } else if (e.control == acc::menus::detail::WagerPopupPanelMoreButton(g_chainPanel)) {
        code = acc::pazaak::kWagerMoreCode;
    } else {
        return false;
    }
    acc::menus::pending::QueueWagerInput(g_chainPanel, code);
    acclog::Write("Menus.Enter", "pazaak-wager-step panel=%p index=%d target=%p code=0x%x",
                  g_chainPanel, g_chainIndex, e.control, code);
    return true;
}

// Adding a companion to a full party: the engine's toggle refuses silently, so
// it still runs (for its click sound) and "Party full" says why. Not for a
// story-locked companion, whose portrait already announced the lock.
bool PartyAddBlocked(PanelKind kind, const ChainEntry& e) {
    if (kind != PanelKind::PartySelection) return false;
    acc::engine::PartyPortrait portrait;
    if (!acc::engine::ReadPartyPortrait(e.control, &portrait)) return false;
    const bool blocked = portrait.selected == 0 &&
                         acc::engine::PartyTableIsNPCSelectable(portrait.npcSlot) &&
                         acc::engine::PartySelectionChosenCount(g_chainPanel) > 1;
    if (!blocked) return false;
    acc::menus::pending::QueueActivate(e.control);
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::PartySelectionFull),
                       /*interrupt=*/false);
    acclog::Write("Menus.Enter", "party-full add blocked panel=%p index=%d target=%p",
                  g_chainPanel, g_chainIndex, e.control);
    return true;
}

// First match wins.
constexpr EnterRoute kEnterRoutes[] = {
    LevelUpStepDisabled, StoreItemRow, CraftRow,   JournalRow,
    QuestItemRow,        TextOnly,     TabButton,  EquipSlot,
    WorkbenchUpgradeSlot, WagerStepButton, PartyAddBlocked,
};

}  // namespace

void HandleEnterActivation(void* activePanel, int code, int val, bool& consumed) {
    if (val == 0) return;
    if (code != kInputEnter1 && code != kInputEnter2) return;
    if (activePanel == nullptr) return;

    // Rebind here too, so a popup the engine pushed and pre-focused (the quit
    // confirm on Cancel, say) activates on Enter without an arrow press first.
    if (g_chainPanel != activePanel) RebindChain(activePanel);
    if (g_chainPanel != activePanel) return;  // the rebind landed elsewhere
    if (g_chainCount <= 0) return;
    if (g_chainIndex < 0 || g_chainIndex >= g_chainCount) return;

    const ChainEntry& e = g_chain[g_chainIndex];
    consumed = true;

    // A virtual entry's control is a sentinel: route it before anything reads it.
    if (e.virtualKind == kVirtualMod_SettingsRoot) {
        acc::menus::modsettings::OpenSubMenu(g_chainPanel);
        acclog::Write("Menus.Enter", "open mod-settings submenu (parent=%p)", g_chainPanel);
        return;
    }
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Enter", "op already pending; ignoring (target=%p)", e.control);
        return;
    }

    const PanelKind kind = acc::engine::IdentifyPanel(g_chainPanel);
    for (EnterRoute route : kEnterRoutes) {
        if (route(kind, e)) return;
    }
    // A sub-screen opened this way arms the drill flag in the
    // OnSwitchToSWInGameGui detour.
    acc::menus::pending::QueueActivate(e.control);
    acclog::Write("Menus.Enter", "activate panel=%p index=%d target=%p",
                  g_chainPanel, g_chainIndex, e.control);
}

}  // namespace acc::menus::chain
