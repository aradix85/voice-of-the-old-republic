// The feat chart of character generation and level-up: what
// menus_skillflow_grid needs to know about CSWGuiFeatsCharGen.

#include "menus_chargen_feats.h"

#include <cstdint>

#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus_internal.h"   // FeatsPanel* resolvers
#include "menus_skillflow_grid.h"

namespace acc::menus::chargen_feats {

namespace {

using acc::menus::detail::FeatsPanelAcceptButton;
using acc::menus::detail::FeatsPanelBackButton;
using acc::menus::detail::FeatsPanelRecommendedButton;
using skillflow_grid::Button;

void* At(void* panel, size_t offset) {
    return static_cast<unsigned char*>(panel) + offset;
}

void* Chart(void* panel)              { return At(panel, kFeatsCharGenChartOffset); }
void* NameLabel(void* panel)          { return At(panel, kFeatsCharGenNameLabelOffset); }
void* DescriptionListBox(void* panel) { return At(panel, kFeatsCharGenDescriptionListBoxOffset); }

// The rows are the chart's own array.
bool ReadRows(void* panel, void** rows, int* count) {
    uint32_t size = 0;
    if (!acc::engine::TryReadPtr(Chart(panel), kSkillFlowChartRowsDataOffset, rows) ||
        !acc::engine::TryReadU32(Chart(panel), kSkillFlowChartRowsSizeOffset, &size)) {
        *rows = nullptr;
        return false;
    }
    *count = static_cast<int>(size);
    return true;
}

typedef void (__thiscall* PFN_PanelFeat)(void* panel, unsigned long featId);

// Refreshes the name label, the description and the Add / Remove button.
void OnEnterFeat(void* panel, unsigned long featId) {
    reinterpret_cast<PFN_PanelFeat>(kAddrCSWGuiFeatsCharGenOnEnterFeat)(panel, featId);
}

void OnFeatPicked(void* panel, unsigned long featId) {
    reinterpret_cast<PFN_PanelFeat>(kAddrCSWGuiFeatsCharGenOnFeatPicked)(panel, featId);
}

// The buttons resolve through the panel's own members, not .gui ids: the ids
// differ between the games (K1's BTN_BACK id is a list box in KOTOR 2).
//
// Each table is one game's on-screen order from left to right, the order the
// generic chain gives the sibling Attributes and Skills screens. The games
// mirror each other; that is a property of the .gui layout, in memory both
// order the members the same.
constexpr Button kRecommended = {FeatsPanelRecommendedButton, "BTN_RECOMMENDED",
                                 acc::strings::Id::ChargenBtnRecommended};
constexpr Button kAccept = {FeatsPanelAcceptButton, "BTN_ACCEPT",
                            acc::strings::Id::ChargenBtnAccept};
constexpr Button kBack = {FeatsPanelBackButton, "BTN_BACK", acc::strings::Id::ChargenBtnBack};
constexpr Button kButtonsK1[] = {kRecommended, kAccept, kBack};
constexpr Button kButtonsK2[] = {kBack, kAccept, kRecommended};

const skillflow_grid::Screen kScreen = {
    "ChargenFeats",
    ReadRows,
    Chart,
    NameLabel,
    DescriptionListBox,
    OnEnterFeat,
    OnFeatPicked,
    acc::strings::Id::FmtChargenFeatUnnamed,
    acc::game::IsKotor2() ? kButtonsK2 : kButtonsK1,
    3,
    FeatsPanelBackButton,
};

skillflow_grid::Grid g_grid(kScreen);

}  // namespace

bool IsChargenFeatsPanel(void* panel) {
    return acc::engine::HasVtable(panel, kVtableCSWGuiFeatsCharGen);
}

bool HandleInput(void* panel, int code, int value, int& outRv) {
    return IsChargenFeatsPanel(panel) && g_grid.HandleInput(panel, code, value, outRv);
}

}  // namespace acc::menus::chargen_feats
