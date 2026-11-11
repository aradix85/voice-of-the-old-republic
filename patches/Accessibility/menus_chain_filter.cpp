// Which controls stay out of the chain: decorative labels and buttons, and the
// per-panel controls that are reached another way.

#include "menus_chain_internal.h"

#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_player.h"   // ReadPartyPortrait / PartyTableIsNPCAvailable
#include "engine_reads.h"
#include "text_match.h"
#include "log.h"
#include "menus_inventory.h"
#include "menus_pazaakdeck.h"
#include "menus_store.h"
#include "engine_offsets_select.h"

namespace acc::menus::chain {

// Panel-aware chain filter: in CSWGuiPortraitCharGen we anchor the chain
// on the left_arrow alone (announces the current portrait via the
// PerKind path in menus_extract.cpp's section 9d) and consolidate the
// right_arrow out of the chain entirely. The user lands on one entry,
// hears the value, and presses Left/Right to cycle — matching the
// existing `[◀] value [▶]` UX without needing the head_3d_scene_control
// (a non-button, IsChainNavigable would reject it) as a chain anchor.
void* ResolvePortraitChargenSkip(void* panel) {
    if (!acc::engine::HasVtable(panel, kVtableCSWGuiPortraitCharGen)) return nullptr;
    return reinterpret_cast<unsigned char*>(panel) + kPortraitRightArrowOffset;
}

// Store-panel action buttons (Close / Sell / Buy).
// These live at fixed struct offsets and we drive them via dedicated
// hotkeys (Esc to close, G to toggle mode, Enter on item to trade)
// rather than chain navigation. Walking past the inventory rows into
// them just adds three dead entries the user has to skip, so filter
// them out of the chain entirely.
void ResolveStoreActionButtons(void* panel, void*& storeCancelBtn,
                               void*& storeToggleBtn,
                               void*& storeAcceptBtn) {
    if (!acc::menus::store::IsStorePanel(panel)) return;
    auto* p = reinterpret_cast<unsigned char*>(panel);
    storeCancelBtn = p + kStoreCancelButtonOffset;
    storeToggleBtn = p + kStoreToggleButtonOffset;
    storeAcceptBtn = p + kStoreAcceptButtonOffset;
}

// InGameEquip picker listbox (LB_ITEMS, id=5): the equip panel keeps
// the picker's item listbox in panel.controls[] even when the picker
// isn't visually shown — the engine pre-populates it with the body-
// slot candidates at panel open. Letting the chain recurse into its
// children leaks rows like "Brejik's Armband" / "Energy Shield" /
// "Sith Energy Shield" into the equip screen between the slot
// buttons. The picker has its own dedicated listbox spec
// (EquipPickerSpec in menus_listbox.cpp) that drives row selection
// when armed; chain nav never needs to step into it. Engine member, not .gui
// id: a variant equip_p.gui renumbers LB_ITEMS and the id lookup then
// excludes the WRONG control, leaking every picker row into the chain.
void* ResolveEquipPickerListBox(void* panel) {
    return IdentifyPanel(panel) == PanelKind::InGameEquip
        ? acc::menus::detail::EquipPanelItemsListBox(panel)
        : nullptr;
}

// WorkbenchUpgrade LB_ITEMS (id=0): the compatible-crystal list. Exactly
// the same hazard as the equip picker above — the engine keeps the listbox
// in panel.controls[] (just toggles its visibility via ShowItems), so the
// generic flatten would expose every crystal row, including rows scrolled
// off the visible page, as standalone chain buttons. Those off-page rows
// are un-presented phantoms: their hit-test returns NULL and FireActivate
// on one AVs the engine (arrowing onto a leaked off-page row after Esc-ing
// the picker shut the game down). The picker has its own spec
// (WorkbenchUpgradeSpec) that drives row selection + scroll safely while
// armed; chain nav must never step into it. Engine member, not .gui id, as
// for the equip picker (docs/gui-id-audit.md).
void* ResolveWorkbenchUpgradeListBox(void* panel) {
    return IdentifyPanel(panel) == PanelKind::WorkbenchUpgrade
        ? acc::menus::detail::UpgradePanelItemsListBox(panel)
        : nullptr;
}

// Per-kind decorative filter. Identifies non-interactive icon buttons
// that the engine drops in panel.controls[] but the user has no reason
// to focus — IsChainNavigable can't tell them from real buttons (same
// CSWGuiButton vtable). Keyed on the panel kind and the control's embedded
// member, never the .gui id: variant .gui files renumber ids, and ids
// collide across the two games.
//
// Currently registered:
//   InGameCharacter btn_3dchar — interaction button for the 3D character
//     model rotator. Image-only with no caption; mouse-drives the model
//     spin, which isn't a screen-reader-useful action. Without the filter
//     it appears as "control N" in the chain.
//   InGameCharacter btn_change1/btn_change2 — party-member switch
//     portraits. The engine cycles party leader on Tab, which re-binds the
//     panel to the new leader and announces the name via
//     party_leader_announce — that covers the same gesture and works
//     in-world too, so these portrait buttons are redundant.
//   InGameCharacter btn_charleft/btn_charright — pagination arrows over
//     the 9-slot NPC roster. Useless in KOTOR 1: max active party is 3
//     (PC + 2 NPCs), and the 2 portrait slots already cover both
//     companions. KOTOR 1 only; the K2 panel has neither.
//   PartySelection accept_button ("Add") — the mouse flow's second
//     step; Enter on a portrait runs the same OnToggled directly.
//   MainMenu warp_button — the developer module-warp button both games
//     ship in the title screen. It jumps into an arbitrary module by
//     resref; there is nothing here for a player, and the engine already
//     hides it visually.
bool IsDecorativeControl(void* panel, void* c,
                         const char* closeCaption, bool haveCloseCaption) {
    PanelKind pk = IdentifyPanel(panel);
    // Pazaak deck builder: drop the overlay value/count/title labels and
    // the unaddable (zero-owned) available cards.
    if (acc::menus::pazaakdeck::IsChainDecorative(panel, c)) return true;
    // Pazaak wager popup: mask the less/more SpeedButtons. The wager is
    // adjusted with Left/Right (held = auto-repeat) via pazaak::Tick's polled
    // stepper, so these buttons are redundant in the chain — dropping them
    // lets Up/Down step straight from the wager row to the wager buttons.
    // Identified by the popup's own members, not by .gui id (the two games
    // number them 4/5 vs 6/7); see WagerPopupPanel* in menus_internal.cpp.
    if (pk == PanelKind::PazaakWager &&
        (c == acc::menus::detail::WagerPopupPanelLessButton(panel) ||
         c == acc::menus::detail::WagerPopupPanelMoreButton(panel))) {
        return true;
    }
    if (pk == PanelKind::InGameCharacter) {
        // Members, not .gui ids: K1's decorative ids 65/66 are K2's BTN_AUTO
        // and BTN_LEVELUP, real actions that must stay navigable. The four
        // party controls exist on K1 only; their offsets poison on K2 and
        // acc::off::Ptr answers nullptr, which no live control can equal.
        if (c == acc::off::Ptr(panel, kCharacterPanel3DCharButtonOffset) ||
            c == acc::off::Ptr(panel, kCharacterPanelChangeParty1ButtonOffset) ||
            c == acc::off::Ptr(panel, kCharacterPanelChangeParty2ButtonOffset) ||
            c == acc::off::Ptr(panel, kCharacterPanelCharLeftButtonOffset) ||
            c == acc::off::Ptr(panel, kCharacterPanelCharRightButtonOffset)) {
            return true;
        }
    }
    // InGameEquip BTN_EQUIP ("OK"): the OK button is the engine's
    // picker-commit button. The accessibility picker dispatcher
    // (menus_listbox.cpp EquipPickerOnEnter) commits the selected item
    // directly via QueueEquipCommit, so OK has no role in chain nav.
    // When the picker isn't armed the engine renders it as "OK,
    // unavailable" — landing on that announces a dead-end. Drop it.
    //
    // InGameEquip BTN_BACK ("Close"): Esc closes the panel via the
    // engine's universal modal-close path, so the close button is
    // functionally redundant for keyboard nav. Same reasoning as
    // Store's Close/Sell/Buy filter above — dedicated
    // hotkey replaces chain landing.
    //
    // Matched by embedded-member offset like the party-cycle strip
    // below, NOT by .gui id: a variant equip_p.gui renumbers these and an
    // id match would then drop a different, live control from the chain.
    //
    // InGameEquip bottom-row party-cycle buttons. Tab cycles the
    // active leader engine-side; the panel re-binds and
    // party_leader_announce speaks the new name. The portrait slots
    // (change_party_1/2) and pagination arrows (character_left/right)
    // are therefore redundant — drop them from the chain.
    if (pk == PanelKind::InGameEquip) {
        auto* p = reinterpret_cast<unsigned char*>(panel);
        if (c == p + kEquipPanelEquipButtonOffset ||
            c == p + kEquipPanelBackButtonOffset ||
            c == p + kEquipPanelChangeParty1ButtonOffset ||
            c == p + kEquipPanelChangeParty2ButtonOffset ||
            c == p + kEquipPanelCharacterLeftButtonOffset ||
            c == p + kEquipPanelCharacterRightButtonOffset) {
            return true;
        }
    }
    // InGameLevelUp "Back" (button_back) and "Cancel"
    // (button_cancel). Both are dead ends for keyboard nav: Back
    // only steps the engine's visual category highlight — we already
    // navigate the level-up categories with our own arrow keys — and
    // Cancel routes to OnCancelPressed, which the engine gates on a
    // can-cancel flag it only ever sets to 0 (see kLevelUpButton*Offset
    // in engine_offsets.h). An in-game level-up cannot be cancelled in
    // vanilla; Accept is the sole exit. Drop both so arrow nav steps
    // only through the actionable category buttons + Accept.
    if (pk == PanelKind::InGameLevelUp) {
        auto* p = reinterpret_cast<unsigned char*>(panel);
        if (c == p + kLevelUpButtonBackOffset ||
            c == p + kLevelUpButtonCancelOffset) {
            return true;
        }
    }
    // InGameMap up_button / down_button ("previous note" /
    // "next note"). They step the engine's GetPrevMapNote /
    // GetNextMapNote cycle, which our map cursor supersedes: W/A/S/D
    // sweeps the map and map_ui_cursor speaks every note it crosses, so
    // the engine's own note stepper is a second, weaker way to do the
    // same thing. Dropping both lets arrow nav step straight to
    // party selection / the return button. The per-kind extractor that
    // names them (menus_extract TryInGameMapArrow) stays — it still
    // labels them for the diagnostic walk and for any focus event the
    // engine raises on its own.
    if (pk == PanelKind::InGameMap) {
        auto* p = reinterpret_cast<unsigned char*>(panel);
        if (c == p + kInGameMapUpButtonOffset ||
            c == p + kInGameMapDownButtonOffset) {
            return true;
        }
    }
    // PartySelection "Add" (BTN_NPC). Enter on a portrait already
    // drives the engine's OnToggled directly, so this button is the
    // mouse flow's redundant second step — landing on it after every
    // portrait just doubles the path to OK.
    if (pk == PanelKind::PartySelection &&
        c == acc::off::Ptr(panel, kPartySelectionPanelAcceptButtonOffset)) {
        return true;
    }
    // MainMenu BTN_WARP. Both ctors clear its visible bit, so a sighted
    // player never sees it — but it stays in panel.controls[], and the chain
    // does not honour that bit, so keyboard nav was the only way to reach a
    // debug jump into an arbitrary module.
    if (pk == PanelKind::MainMenu &&
        c == acc::off::Ptr(panel, kMainMenuWarpButtonOffset)) {
        return true;
    }
    // Inventory "Use" (useitem_button). Same redundancy as the
    // PartySelection Add above, plus a correctness problem of its own:
    // the button is the engine's gamepad-A shortcut, so it dispatches
    // activate to panel.active_control rather than to our chain focus, and
    // our cursor warp usually fails to land on an item row (the hit test
    // returns NULL for them). Pressing it therefore fires whatever row
    // the engine still considers active, which can be a different item than
    // the one the user is standing on. Enter on the row reaches the same
    // per-item handler (CreateItemEntry AddEvent's OnControlSelected /
    // CantEquip / NotUseable / FullHealth / … on the row itself), directly
    // and unambiguously — so the button is pure hazard for keyboard nav.
    if (pk == PanelKind::InGameInventory &&
        acc::menus::inventory::IsUseItemButton(panel, c)) {
        return true;
    }
    // PartySelection portraits with no currently-selectable
    // companion. The panel renders every roster slot in a fixed grid
    // (9 on KOTOR 1, 12 on KOTOR 2); sighted players see empty / greyed
    // slots, but a blind navigator has nothing actionable on them (the
    // engine refuses Add/OK anyway) and the spoiler rule from
    // menus_extract section 7b means we deliberately don't speak
    // a name. Treating them as decorative drops them from the
    // chain entirely so arrow keys only step through usable picks.
    //
    // The KOTOR 1 flag word is NOT a reliable gate: slots 6/7/8 read values
    // like 0xfffffff9 / 0x5f484c41 / 0x39000001 (uninitialised heap whose
    // low bit randomly happens to be 1). The roster index IS
    // reliable on both games, so route the decision through the
    // engine: keep the portrait if the slot is in the active
    // party (partyId >= 0) OR the engine's own GetIsNPCAvailable
    // returns true for that roster index.
    if (pk == PanelKind::PartySelection) {
        PartyPortrait portrait;
        if (ReadPartyPortrait(c, &portrait) &&
            portrait.partyId < 0 &&
            !PartyTableIsNPCAvailable(portrait.npcSlot)) {
            return true;
        }
    }
    // WorkbenchUpgrade slot buttons (matched by position in the panel's
    // embedded button run — UpgradeSlotIndexFromButton) that the engine
    // has marked non-interactive (bit_flags & 0x2 == 0). For a 3-slot
    // ranged weapon (saber=3) these are the 4 crystal positions;
    // for a 4-slot saber/double-shaft (saber=2) they are the 3
    // upgrade positions. Sighted players see them greyed; a
    // keyboard navigator has nothing to do with them — OnSlotSelected
    // gates on is_active so even pressing Enter does nothing useful.
    // Dropping them from the chain lets arrow-down go straight from
    // the last applicable slot to BTN_ASSEMBLE.
    if (pk == PanelKind::WorkbenchUpgrade &&
        acc::menus::detail::UpgradeSlotIndexFromButton(panel, c) >= 0) {
        if ((acc::engine::ReadControlBitFlags(c) & 0x2) == 0) return true;
    }
    // Universal close-button filter. Every
    // standalone "close/back" button the engine ships across
    // sub-screens — whatever the .gui names it (BTN_EXIT / BTN_BACK /
    // BTN_CANCEL / BTN_Cancel) — renders caption strref 1582. Esc
    // already dismisses each of these panels (HandleEsc →
    // FindCancelButton/FindCloseButton scan panel.controls directly, so
    // they still find it after we drop it from the chain), making the
    // button redundant for keyboard nav. Match the engine's *resolved*
    // 1582 text rather than the +0x174 strref field — the engine
    // renders these captions via gui_string and frequently leaves the
    // strref slot empty, so a text compare is the reliable signal.
    // Gated to plain buttons (AsButton): close buttons are never
    // toggles/sliders, and this keeps ReadButtonText off slider structs.
    // This subsumes the per-panel Store/Equip/Options Close filters
    // above; those stay for their non-1582 siblings (Equip OK, Store
    // Sell/Buy) and as the Esc-routing anchors.
    //
    // EqualsTrimmed, not strcmp: KOTOR 2 renders this caption as
    // "\x11 Close" — an Aspyr controller-glyph marker, then a pad space,
    // then the word — against a dialog.tlk that says "Close", so a
    // byte-exact compare never matched on K2. ReadGuiString takes the 0x11
    // off, this takes the space off; both halves are needed. KOTOR 1 emits
    // neither.
    if (haveCloseCaption &&
        CallDowncast(c, kVtableAsButton) != nullptr) {
        char btnText[64];
        if (ReadButtonText(c, btnText, sizeof(btnText)) &&
            acc::text::EqualsTrimmed(btnText, closeCaption)) {
            acclog::Write("Menus.Chain",
                          "filter close button panel=%p ctrl=%p "
                          "text=\"%s\" (TLK %u)",
                          panel, c, btnText, kCloseButtonStrRef);
            return true;
        }
    }
    return false;
}

}  // namespace acc::menus::chain
