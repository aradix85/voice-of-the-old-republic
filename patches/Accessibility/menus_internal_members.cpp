// The panels' controls by their ctor-bound members (equipment, workbench, save/load, crafting,
// powers, container, feats and the rest), with the .gui id kept as a tripwire, and the slot index
// of an equipment or upgrade slot button.

#include "menus_internal_internal.h"

#include "log.h"
#include "engine_game.h"      // IsKotor2 — the save/load control ids differ
#include "engine_offsets.h"

namespace acc::menus::detail {

namespace {

// Equip-panel slot identity via the engine's embedded control arrays —
// see the menus_internal.h comment and docs/gui-id-audit.md. Pure
// pointer arithmetic; the control pointer is never dereferenced, so an
// unrelated control simply lands outside the array (or off-stride) and
// answers -1.

int SlotIndexInEmbeddedArray(void* panel, void* control,
                             size_t baseOff, size_t stride) {
    if (!panel || !control) return -1;
    if (!acc::off::Ok(baseOff) || !acc::off::Ok(stride) || stride == 0)
        return -1;
    uintptr_t base = reinterpret_cast<uintptr_t>(panel) + baseOff;
    uintptr_t c    = reinterpret_cast<uintptr_t>(control);
    if (c < base) return -1;
    uintptr_t delta = c - base;
    if (delta % stride != 0) return -1;
    int idx = static_cast<int>(delta / stride);
    return idx < acc::menus::detail::EquipSlotCount() ? idx : -1;
}

// Engine-truth resolvers for the equip / workbench-upgrade picker
// members. The member address is the answer; the old .gui-id lookup stays
// only as a tripwire so a variant .gui file shows up in tester logs
// instead of silently resolving wrong (userlogs/077noequipment is exactly
// that log line's audience). acclog::Once caps it at one line per tag per
// session. guiId < 0 skips the tripwire — used where the historical id is
// known to be ambiguous on one game (K2 reuses the workbench assemble id
// inside its slot-button id set).

void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId,
                              const char* tag) {
    void* member = acc::off::Ptr(panel, memberOff);
    if (!member) return nullptr;
    if (guiId >= 0) {
        void* byId = acc::menus::detail::FindControlById(panel, guiId);
        if (byId && byId != member) {
            acclog::Once(tag, "GuiIdMismatch: %s member=%p but .gui id %d "
                         "resolves to %p — variant .gui file on this install",
                         tag, member, guiId, byId);
        }
    }
    return member;
}

// K2 workbench/crafting resolvers (see menus_internal.h). The tripwire ids
// are the historical hardcoded ones, mined from K2's own gui.bif: the two
// crafting .gui files agree on the listbox/accept ids (4/5/12) but disagree
// on BTN_Examine (component_p 13, chemical_p 14), which is why the examine
// tripwire follows the panel kind. The offsets poison on K1, so every
// resolver returns nullptr there without touching the tripwire.

// The two crafting classes lay the same controls out at different offsets;
// pick per panel kind, or return nullptr when `panel` is neither.
void* CraftMemberWithTripwire(void* panel, size_t componentOff,
                              size_t chemicalOff, int guiId,
                              const char* tag) {
    switch (acc::engine::IdentifyPanel(panel)) {
        case acc::engine::PanelKind::WorkbenchCreateItem:
            return PanelMemberWithTripwire(panel, componentOff, guiId, tag);
        case acc::engine::PanelKind::WorkbenchCreateMedical:
            return PanelMemberWithTripwire(panel, chemicalOff, guiId, tag);
        default:
            return nullptr;
    }
}

}  // namespace

// CSWGuiSaveLoad control ids from saveload.gui / K2 saveload_p.gui
// (verified against chain logs: patch-20260505-160124.log lines 45-65;
// K2 witnessed live in patch-20260813-212757 — K2 re-authored the screen
// and every id but the action button moved, which once silently disabled
// the whole SaveLoad spec there).
//
// SINCE the gui-id audit (2026-08-18, docs/gui-id-audit.md) these ids are
// NOT how the panel or its controls are resolved any more: identification
// is the CSWGuiSaveLoad vtable, and the controls are the ctor-bound
// embedded members (kSaveLoadPanel*Offset). The ids survive only as the
// GuiIdMismatch tripwire input on the SaveLoadPanel* resolvers below, so
// a variant .gui file shows up as one loud log line instead of a
// mis-wired screen.
//
//   K1                              K2
//   id=0   games_listbox            id=12  games_listbox
//   id=12  back_button              id=13  back_button
//   id=14  saveload_button          id=14  saveload_button (unchanged)
inline int SaveLoadLbGamesId() { return acc::game::IsKotor2() ? 12 :  0; }

inline int SaveLoadBtnBackId() { return acc::game::IsKotor2() ? 13 : 12; }

inline int SaveLoadBtnSaveLoadId() { return 14; }

int EquipSlotIndexFromButton(void* panel, void* control) {
    return SlotIndexInEmbeddedArray(panel, control,
                                    kEquipPanelSlotButtonsOffset,
                                    kEquipPanelSlotButtonStride);
}

int EquipSlotIndexFromControl(void* panel, void* control) {
    int idx = EquipSlotIndexFromButton(panel, control);
    if (idx >= 0) return idx;
    return SlotIndexInEmbeddedArray(panel, control,
                                    kEquipPanelSlotLabelsOffset,
                                    kEquipPanelSlotLabelStride);
}

void* EquipPanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelItemsListBoxOffset,
                                   kEquipLbItemsId, "Equip.LB_ITEMS");
}

void* EquipPanelEquipButton(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelEquipButtonOffset,
                                   kEquipBtnEquipId, "Equip.BTN_EQUIP");
}

void* EquipPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelBackButtonOffset,
                                   kEquipBtnBackId, "Equip.BTN_BACK");
}

void* UpgradePanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradePanelItemsListBoxOffset,
                                   kWorkbenchUpgradeLbItemsId,
                                   "Upgrade.LB_ITEMS");
}

void* UpgradePanelAssembleButton(void* panel) {
    // No id tripwire: the historical id 24 is a K2 slot-button id too, so
    // an id lookup is ambiguous there by construction.
    return PanelMemberWithTripwire(panel, kUpgradePanelAssembleButtonOffset,
                                   acc::game::IsKotor2() ? -1 : 24,
                                   "Upgrade.BTN_ASSEMBLE");
}

void* UpgradePanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradePanelBackButtonOffset,
                                   kWorkbenchUpgradeBtnBackId,
                                   "Upgrade.BTN_BACK");
}

// CSWGuiSaveLoad — same engine-truth resolvers (members mined from both
// games' ctors, see kSaveLoadPanel*Offset). Callers should have vetted the
// panel with IsSaveLoadPanel first; the member address is pure pointer
// arithmetic either way.
void* SaveLoadPanelGamesListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelGamesListBoxOffset,
                                   SaveLoadLbGamesId(), "SaveLoad.LB_GAMES");
}

void* SaveLoadPanelActionButton(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelActionButtonOffset,
                                   SaveLoadBtnSaveLoadId(),
                                   "SaveLoad.BTN_SAVELOAD");
}

void* SaveLoadPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelBackButtonOffset,
                                   SaveLoadBtnBackId(), "SaveLoad.BTN_BACK");
}

void* UpgradeSelPanelListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradeSelPanelListBoxOffset,
                                   /*LB_UPGRADELIST=*/9,
                                   "UpgradeSel.LB_UPGRADELIST");
}

void* UpgradeSelPanelUpgradeButton(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradeSelPanelUpgradeButtonOffset,
                                   /*BTN_UPGRADEITEMS=*/5,
                                   "UpgradeSel.BTN_UPGRADEITEMS");
}

void* UpgradeSelPanelTitleLabel(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradeSelPanelTitleLabelOffset,
                                   /*LBL_TITLE=*/4, "UpgradeSel.LBL_TITLE");
}

void* CraftPanelShopListBox(void* panel) {
    return CraftMemberWithTripwire(panel, kCraftComponentShopListBoxOffset,
                                   kCraftChemicalShopListBoxOffset,
                                   /*LB_SHOPITEMS=*/4, "Craft.LB_SHOPITEMS");
}

void* CraftPanelInvListBox(void* panel) {
    return CraftMemberWithTripwire(panel, kCraftComponentInvListBoxOffset,
                                   kCraftChemicalInvListBoxOffset,
                                   /*LB_INVITEMS=*/5, "Craft.LB_INVITEMS");
}

void* CraftPanelAcceptButton(void* panel) {
    return CraftMemberWithTripwire(panel, kCraftComponentAcceptButtonOffset,
                                   kCraftChemicalAcceptButtonOffset,
                                   /*BTN_Accept=*/12, "Craft.BTN_Accept");
}

void* CraftPanelExamineButton(void* panel) {
    bool medical = acc::engine::IdentifyPanel(panel) ==
                   acc::engine::PanelKind::WorkbenchCreateMedical;
    return CraftMemberWithTripwire(panel, kCraftComponentExamineButtonOffset,
                                   kCraftChemicalExamineButtonOffset,
                                   /*BTN_Examine=*/medical ? 14 : 13,
                                   "Craft.BTN_Examine");
}

void* CraftPanelTitleLabel(void* panel) {
    bool medical = acc::engine::IdentifyPanel(panel) ==
                   acc::engine::PanelKind::WorkbenchCreateMedical;
    return CraftMemberWithTripwire(panel, kCraftComponentTitleLabelOffset,
                                   kCraftChemicalTitleLabelOffset,
                                   /*LBL_TITLE=*/medical ? 15 : 19,
                                   "Craft.LBL_TITLE");
}

// CSWGuiPowersLevelUp resolvers (see menus_internal.h). The tripwire ids
// are the per-game historical ones mined from each game's own gui.bif —
// the two games re-number pwrlvlup's controls AND collide (K1 id 6 is
// LB_POWERS while K2 id 6 is a label; K1 id 12 is BTN_BACK while K2 id 12
// is LB_POWERS), which is exactly the fragility this audit removes.
void* PowersPanelPowersListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelPowersListBoxOffset,
                                   acc::game::IsKotor2() ? 12 : 6,
                                   "Powers.LB_POWERS");
}

void* PowersPanelDescListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelDescListBoxOffset,
                                   acc::game::IsKotor2() ? 3 : 7,
                                   "Powers.LB_DESC");
}

void* PowersPanelPowerLabel(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelPowerLabelOffset,
                                   acc::game::IsKotor2() ? 4 : 8,
                                   "Powers.LBL_POWER");
}

void* PowersPanelSubTitleLabel(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelSubTitleLabelOffset,
                                   /*SUB_TITLE_LBL, both games*/ 1,
                                   "Powers.SUB_TITLE_LBL");
}

void* PowersPanelRecommendedButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelRecommendedButtonOffset,
                                   acc::game::IsKotor2() ? 11 : 9,
                                   "Powers.RECOMMENDED_BTN");
}

void* PowersPanelAcceptButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelAcceptButtonOffset,
                                   acc::game::IsKotor2() ? 10 : 11,
                                   "Powers.ACCEPT_BTN");
}

void* PowersPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPowersPanelBackButtonOffset,
                                   acc::game::IsKotor2() ? 9 : 12,
                                   "Powers.BACK_BTN");
}

// CSWGuiContainer resolvers (see menus_internal.h). container.gui and
// container_p.gui number these identically, so one tripwire id per
// control covers both games.
void* ContainerPanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kContainerPanelItemsListBoxOffset,
                                   /*LB_ITEMS=*/2, "Container.LB_ITEMS");
}

void* ContainerPanelOkButton(void* panel) {
    return PanelMemberWithTripwire(panel, kContainerPanelOkButtonOffset,
                                   /*BTN_OK=*/3, "Container.BTN_OK");
}

void* ContainerPanelCancelButton(void* panel) {
    return PanelMemberWithTripwire(panel, kContainerPanelCancelButtonOffset,
                                   /*BTN_CANCEL=*/5, "Container.BTN_CANCEL");
}

void* ContainerPanelGiveItemsButton(void* panel) {
    return PanelMemberWithTripwire(panel, kContainerPanelGiveItemsButtonOffset,
                                   /*BTN_GIVEITEMS=*/4,
                                   "Container.BTN_GIVEITEMS");
}

// CSWGuiFeatsCharGen resolvers (see menus_internal.h). The tripwire ids
// are each game's own ftchrgen numbering: K1 9/11/12 =
// Recommended/Accept/Back, K2 9/10/11 = Back/Accept/Recommended. K1's
// BTN_BACK id 12 lands on K2's LB_FEATS listbox, which is what made the
// old id table mis-fire on KOTOR 2 (patch-20260803-011930.log: reading a
// listbox as a button spoke a stray str_ref and Enter activated the
// listbox instead of Cancel).
void* FeatsPanelAcceptButton(void* panel) {
    return PanelMemberWithTripwire(panel, kFeatsCharGenAcceptButtonOffset,
                                   acc::game::IsKotor2() ? 10 : 11,
                                   "Feats.BTN_ACCEPT");
}

void* FeatsPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kFeatsCharGenBackButtonOffset,
                                   acc::game::IsKotor2() ? 9 : 12,
                                   "Feats.BTN_BACK");
}

void* FeatsPanelRecommendedButton(void* panel) {
    return PanelMemberWithTripwire(panel, kFeatsCharGenRecommendedButtonOffset,
                                   acc::game::IsKotor2() ? 11 : 9,
                                   "Feats.BTN_RECOMMENDED");
}

// CSWGuiSkillInfoBox resolvers (see menus_internal.h). skillinfo.gui and
// skillinfo_p.gui agree on the ids, so the same tripwire serves both games.
void* SkillInfoBoxPanelSkillsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kSkillInfoBoxPanelSkillsListBoxOffset,
                                   2, "SkillInfoBox.LB_SKILLS");
}

void* SkillInfoBoxPanelMessageLabel(void* panel) {
    return PanelMemberWithTripwire(panel, kSkillInfoBoxPanelMessageLabelOffset,
                                   0, "SkillInfoBox.LBL_MESSAGE");
}

void* SkillInfoBoxPanelOkButton(void* panel) {
    return PanelMemberWithTripwire(panel, kSkillInfoBoxPanelOkButtonOffset,
                                   4, "SkillInfoBox.BTN_OK");
}

// CSWGuiScriptSelect resolvers (see menus_internal.h). KOTOR 1 only — the
// offsets poison on KOTOR 2 and acc::off::Ptr answers nullptr there, which
// is the same answer the vtable-gated callers already expect.
void* ScriptSelectPanelAiStateListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kScriptSelectPanelAiStateListBoxOffset,
                                   0, "ScriptSelect.LST_AIState");
}

void* ScriptSelectPanelAcceptButton(void* panel) {
    return PanelMemberWithTripwire(panel, kScriptSelectPanelAcceptButtonOffset,
                                   4, "ScriptSelect.BTN_Accept");
}

void* ScriptSelectPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kScriptSelectPanelBackButtonOffset,
                                   3, "ScriptSelect.BTN_Back");
}

// CSWGuiWagerPopup resolvers (see menus_internal.h). The two games number
// this .gui differently, so each tripwire id is per game.
void* WagerPopupPanelLessButton(void* panel) {
    return PanelMemberWithTripwire(panel, kWagerPopupPanelLessButtonOffset,
                                   acc::game::IsKotor2() ? 6 : 4,
                                   "Wager.BTN_LESS");
}

void* WagerPopupPanelMoreButton(void* panel) {
    return PanelMemberWithTripwire(panel, kWagerPopupPanelMoreButtonOffset,
                                   acc::game::IsKotor2() ? 7 : 5,
                                   "Wager.BTN_MORE");
}

void* WagerPopupPanelMaximumLabel(void* panel) {
    return PanelMemberWithTripwire(panel, kWagerPopupPanelMaximumLabelOffset,
                                   acc::game::IsKotor2() ? 2 : 3,
                                   "Wager.LBL_MAXIMUM");
}

// CSWGuiPazaakStart resolvers (see menus_internal.h).
void* PazaakStartPanelPlayButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPazaakStartPanelPlayButtonOffset,
                                   acc::game::IsKotor2() ? 95 : 78,
                                   "PazaakDeck.BTN_ATEXT");
}

void* PazaakStartPanelClearButton(void* panel) {
    // KOTOR 1's pazaaksetup.gui has no clear-deck button; the offset poisons
    // there and Ptr answers nullptr, so the caller's row simply loses that
    // entry (which is how the id -1 behaved before).
    return PanelMemberWithTripwire(panel, kPazaakStartPanelClearButtonOffset,
                                   acc::game::IsKotor2() ? 96 : -1,
                                   "PazaakDeck.BTN_CLEARCARDS");
}

// CSWGuiInGameOptKeyMappings resolvers (see menus_internal.h). The
// tripwire ids are optkeymapping.gui's; KOTOR 2's file was never mined, so
// if optkeymapping_p renumbers, the mismatch lines are the report of that
// rather than a broken screen — the member path is unaffected either way.
void* KeyMapPanelEventListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kKeyMapPanelEventListBoxOffset,
                                   0, "KeyMap.LST_EventList");
}

void* KeyMapPanelDefaultButton(void* panel) {
    return PanelMemberWithTripwire(panel, kKeyMapPanelDefaultButtonOffset,
                                   2, "KeyMap.BTN_Default");
}

void* KeyMapPanelAcceptButton(void* panel) {
    return PanelMemberWithTripwire(panel, kKeyMapPanelAcceptButtonOffset,
                                   3, "KeyMap.BTN_Accept");
}

void* KeyMapPanelCancelButton(void* panel) {
    return PanelMemberWithTripwire(panel, kKeyMapPanelCancelButtonOffset,
                                   4, "KeyMap.BTN_Cancel");
}

void* KeyMapPanelFilterButton(void* panel, int filterIndex) {
    if (filterIndex < 0 || filterIndex > 2) return nullptr;
    if (!acc::off::Ok(kKeyMapPanelFilterButtonsOffset) ||
        !acc::off::Ok(kKeyMapPanelFilterButtonStride)) {
        return nullptr;
    }
    static const char* const kTags[3] = { "KeyMap.BTN_Filter_Move",
                                          "KeyMap.BTN_Filter_Game",
                                          "KeyMap.BTN_Filter_Mini" };
    const size_t off = kKeyMapPanelFilterButtonsOffset +
                       static_cast<size_t>(filterIndex) *
                           kKeyMapPanelFilterButtonStride;
    return PanelMemberWithTripwire(panel, off, 6 + filterIndex,
                                   kTags[filterIndex]);
}

// CSWGuiUpgradeItemSelect resolvers (see menus_internal.h). upgradeitems.gui
// and upgradeitems_p.gui agree on the ids (LB_ITEMS 0, BTN_UPGRADEITEM 4,
// BTN_BACK 5), so one tripwire id serves both games.
void* WorkbenchItemsPanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kWorkbenchItemsPanelItemsListBoxOffset,
                                   0, "WorkbenchItems.LB_ITEMS");
}

void* WorkbenchItemsPanelUpgradeButton(void* panel) {
    return PanelMemberWithTripwire(panel, kWorkbenchItemsPanelUpgradeButtonOffset,
                                   4, "WorkbenchItems.BTN_UPGRADEITEM");
}

void* WorkbenchItemsPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kWorkbenchItemsPanelBackButtonOffset,
                                   5, "WorkbenchItems.BTN_BACK");
}

// Workbench-upgrade slot membership via the panel's embedded button run
// (one contiguous array in both games — see the kUpgradePanelSlotButtons
// note in engine_offsets_fields.h). Returns the ARRAY index (identity /
// membership only): the engine's slot semantics live in the button's
// custom_value, which is per-bank, so callers keep reading that.
int UpgradeSlotIndexFromButton(void* panel,
                                                   void* control) {
    if (!panel || !control) return -1;
    if (!acc::off::Ok(kUpgradePanelSlotButtonsOffset) ||
        !acc::off::Ok(kUpgradePanelSlotButtonStride)) return -1;
    uintptr_t base = reinterpret_cast<uintptr_t>(panel) +
                     kUpgradePanelSlotButtonsOffset;
    uintptr_t c = reinterpret_cast<uintptr_t>(control);
    if (c < base) return -1;
    uintptr_t delta = c - base;
    if (delta % kUpgradePanelSlotButtonStride != 0) return -1;
    int idx = static_cast<int>(delta / kUpgradePanelSlotButtonStride);
    return idx < UpgradeSlotButtonCount() ? idx : -1;
}

}  // namespace acc::menus::detail
