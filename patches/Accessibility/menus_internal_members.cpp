// The panel-member resolvers (see menus_internal.h) and the slot index of an
// equipment or workbench-upgrade slot button.

#include <cstdint>

#include "menus_internal.h"
#include "log.h"
#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_panels.h"

using namespace acc::engine;

namespace acc::menus::detail {

namespace {

// Index of `control` in the run of `count` controls embedded in `panel` at
// baseOff, or -1. Pure pointer arithmetic: the control is never
// dereferenced, so an unrelated control lands outside the run or off-stride.
int SlotIndexInEmbeddedArray(void* panel, void* control, size_t baseOff,
                             size_t stride, int count) {
    if (!panel || !control) return -1;
    if (!acc::off::Ok(baseOff) || !acc::off::Ok(stride) || stride == 0)
        return -1;
    uintptr_t base = reinterpret_cast<uintptr_t>(panel) + baseOff;
    uintptr_t c    = reinterpret_cast<uintptr_t>(control);
    if (c < base) return -1;
    uintptr_t delta = c - base;
    if (delta % stride != 0) return -1;
    int idx = static_cast<int>(delta / stride);
    return idx < count ? idx : -1;
}

// guiId < 0 skips the tripwire, for an id that is ambiguous on the running
// game.
void* PanelMemberWithTripwire(void* panel, size_t memberOff, int guiId,
                              const char* tag) {
    void* member = acc::off::Ptr(panel, memberOff);
    if (!member) return nullptr;
    if (guiId >= 0) {
        void* byId = FindControlById(panel, guiId);
        if (byId && byId != member) {
            acclog::Once(tag, "GuiIdMismatch: %s member=%p but .gui id %d "
                         "resolves to %p — variant .gui file on this install",
                         tag, member, guiId, byId);
        }
    }
    return member;
}

// The member for whichever crafting class `panel` is, or nullptr.
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

bool IsMedicalCraftPanel(void* panel) {
    return acc::engine::IdentifyPanel(panel) ==
           acc::engine::PanelKind::WorkbenchCreateMedical;
}

}  // namespace

int EquipSlotIndexFromButton(void* panel, void* control) {
    return SlotIndexInEmbeddedArray(panel, control,
                                    kEquipPanelSlotButtonsOffset,
                                    kEquipPanelSlotButtonStride,
                                    EquipSlotCount());
}

int EquipSlotIndexFromControl(void* panel, void* control) {
    int idx = EquipSlotIndexFromButton(panel, control);
    if (idx >= 0) return idx;
    return SlotIndexInEmbeddedArray(panel, control,
                                    kEquipPanelSlotLabelsOffset,
                                    kEquipPanelSlotLabelStride,
                                    EquipSlotCount());
}

int UpgradeSlotIndexFromButton(void* panel, void* control) {
    return SlotIndexInEmbeddedArray(panel, control,
                                    kUpgradePanelSlotButtonsOffset,
                                    kUpgradePanelSlotButtonStride,
                                    acc::game::IsKotor2() ? 9 : 7);
}

// The tripwire ids are the historical .gui ids, per game where the two games
// number a file differently.

void* EquipPanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelItemsListBoxOffset,
                                   acc::game::IsKotor2() ? 41 : 5,
                                   "Equip.LB_ITEMS");
}

void* EquipPanelEquipButton(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelEquipButtonOffset,
                                   acc::game::IsKotor2() ? 40 : 37,
                                   "Equip.BTN_EQUIP");
}

void* EquipPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kEquipPanelBackButtonOffset,
                                   acc::game::IsKotor2() ? 39 : 36,
                                   "Equip.BTN_BACK");
}

void* UpgradePanelItemsListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradePanelItemsListBoxOffset,
                                   0, "Upgrade.LB_ITEMS");
}

void* UpgradePanelAssembleButton(void* panel) {
    // No tripwire on KOTOR 2: the assemble id 24 is one of its slot-button
    // ids too.
    return PanelMemberWithTripwire(panel, kUpgradePanelAssembleButtonOffset,
                                   acc::game::IsKotor2() ? -1 : 24,
                                   "Upgrade.BTN_ASSEMBLE");
}

void* UpgradePanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kUpgradePanelBackButtonOffset,
                                   acc::game::IsKotor2() ? 13 : 28,
                                   "Upgrade.BTN_BACK");
}

void* SaveLoadPanelGamesListBox(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelGamesListBoxOffset,
                                   acc::game::IsKotor2() ? 12 : 0,
                                   "SaveLoad.LB_GAMES");
}

void* SaveLoadPanelActionButton(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelActionButtonOffset,
                                   14, "SaveLoad.BTN_SAVELOAD");
}

void* SaveLoadPanelBackButton(void* panel) {
    return PanelMemberWithTripwire(panel, kSaveLoadPanelBackButtonOffset,
                                   acc::game::IsKotor2() ? 13 : 12,
                                   "SaveLoad.BTN_BACK");
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

// component_p and chemical_p agree on the listbox and accept ids but not on
// BTN_Examine and LBL_TITLE.
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
    return CraftMemberWithTripwire(panel, kCraftComponentExamineButtonOffset,
                                   kCraftChemicalExamineButtonOffset,
                                   IsMedicalCraftPanel(panel) ? 14 : 13,
                                   "Craft.BTN_Examine");
}

void* CraftPanelTitleLabel(void* panel) {
    return CraftMemberWithTripwire(panel, kCraftComponentTitleLabelOffset,
                                   kCraftChemicalTitleLabelOffset,
                                   IsMedicalCraftPanel(panel) ? 15 : 19,
                                   "Craft.LBL_TITLE");
}

// The two games number pwrlvlup differently and collide: K1 id 6 is
// LB_POWERS where K2 id 6 is a label, K1 id 12 is BACK_BTN where K2 id 12 is
// LB_POWERS.
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
                                   1, "Powers.SUB_TITLE_LBL");
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

// ftchrgen: K1 9/11/12 = Recommended/Accept/Back, K2 9/10/11 =
// Back/Accept/Recommended. K1's BTN_BACK id 12 is K2's LB_FEATS listbox.
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

void* PazaakStartPanelPlayButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPazaakStartPanelPlayButtonOffset,
                                   acc::game::IsKotor2() ? 95 : 78,
                                   "PazaakDeck.BTN_ATEXT");
}

void* PazaakStartPanelClearButton(void* panel) {
    return PanelMemberWithTripwire(panel, kPazaakStartPanelClearButtonOffset,
                                   acc::game::IsKotor2() ? 96 : -1,
                                   "PazaakDeck.BTN_CLEARCARDS");
}

// The ids are optkeymapping.gui's; KOTOR 2's file was never mined, so a
// renumbered optkeymapping_p shows up as mismatch lines, not a broken screen.
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

}  // namespace acc::menus::detail
