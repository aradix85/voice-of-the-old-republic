// The Force-power chart: what menus_skillflow_grid needs to know about
// CSWGuiPowersLevelUp.

#include "menus_powers_levelup.h"

#include <cstdint>

#include "engine_offsets.h"
#include "engine_panels.h"    // IdentifyPanel
#include "engine_reads.h"
#include "menus_internal.h"   // PowersPanel* resolvers
#include "menus_skillflow_grid.h"

namespace acc::menus::powers_levelup {

namespace {

using acc::menus::detail::PowersPanelAcceptButton;
using acc::menus::detail::PowersPanelBackButton;
using acc::menus::detail::PowersPanelRecommendedButton;
using skillflow_grid::Button;

void* Chart(void* panel) {
    return static_cast<unsigned char*>(panel) + kPowersLevelUpChartOffset;
}

// The rows are the controls of the powers list box. The chart only holds the
// selection here; its own row array is not filled in this screen
// (CSWGuiPowersLevelUp::OnPowerSelectionChanged @0x006f1940 takes the
// CSWGuiSkillFlow from the list box's selected control).
bool ReadRows(void* panel, void** rows, int* count) {
    void* listBox = acc::menus::detail::PowersPanelPowersListBox(panel);
    *count = acc::engine::ReadListBoxRowCount(listBox);
    if (*count < 0 || !acc::engine::TryReadPtr(listBox, kListBoxControlsOffset, rows)) {
        *rows = nullptr;
        *count = 0;
        return false;
    }
    return true;
}

typedef void (__thiscall* PFN_PanelPower)(void* panel, unsigned long powerId);

void OnEnterPower(void* panel, unsigned long powerId) {
    reinterpret_cast<PFN_PanelPower>(kAddrCSWGuiPowersLevelUpOnEnterPower)(panel, powerId);
}

void OnPowerPicked(void* panel, unsigned long powerId) {
    reinterpret_cast<PFN_PanelPower>(kAddrCSWGuiPowersLevelUpOnPowerPicked)(panel, powerId);
}

// The Add Power button is left out: Enter on a cell calls OnPowerPicked,
// which is what that button's click does.
constexpr Button kButtons[] = {
    {PowersPanelRecommendedButton, "RECOMMENDED_BTN", acc::strings::Id::ChargenBtnRecommended},
    {PowersPanelAcceptButton,      "ACCEPT_BTN",      acc::strings::Id::ChargenBtnAccept},
    {PowersPanelBackButton,        "BACK_BTN",        acc::strings::Id::ChargenBtnBack},
};

const skillflow_grid::Screen kScreen = {
    "PowersLevelUp",
    ReadRows,
    Chart,
    acc::menus::detail::PowersPanelPowerLabel,
    acc::menus::detail::PowersPanelDescListBox,
    OnEnterPower,
    OnPowerPicked,
    acc::strings::Id::FmtChargenPowerUnnamed,
    kButtons,
    3,
    PowersPanelBackButton,
};

skillflow_grid::Grid g_grid(kScreen);

}  // namespace

bool IsPowersLevelUpPanel(void* panel) {
    return acc::engine::IdentifyPanel(panel) == acc::engine::PanelKind::PowersLevelUp;
}

const char* GetTitleOverride(void* panel) {
    if (!IsPowersLevelUpPanel(panel)) return nullptr;
    static thread_local char s_title[128];
    void* label = acc::menus::detail::PowersPanelSubTitleLabel(panel);
    if (!label || !acc::engine::ReadLabelText(label, s_title, sizeof(s_title)) || !s_title[0]) {
        return nullptr;
    }
    return s_title;
}

bool HandleInput(void* panel, int code, int value, int& outRv) {
    return IsPowersLevelUpPanel(panel) && g_grid.HandleInput(panel, code, value, outRv);
}

}  // namespace acc::menus::powers_levelup
