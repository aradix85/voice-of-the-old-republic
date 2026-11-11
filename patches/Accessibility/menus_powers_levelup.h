// The Force-power picker (CSWGuiPowersLevelUp), in character generation and
// in level-up: one engine class hosts both.
//
// The panel is a chart of power families, not a flat list: its powers list
// box holds CSWGuiSkillFlow rows with up to three cells (the power, its
// improved and its master form), the same layout as the Feats screen. The
// keyboard model is the shared one in menus_skillflow_grid.h.

#pragma once

namespace acc::menus::powers_levelup {

bool IsPowersLevelUpPanel(void* panel);

// The title to announce for this panel: its sub-title label, which names the
// powers screen, where the main title label of pwrlvlup.gui is the character
// generation header. nullptr when `panel` is another screen. The buffer is
// thread-local: use it before the next call.
const char* GetTitleOverride(void* panel);

// True when the event is consumed; `outRv` is then the input hook's result.
bool HandleInput(void* panel, int code, int value, int& outRv);

}  // namespace acc::menus::powers_levelup
