// The Feats screen of character generation and level-up (CSWGuiFeatsCharGen).
//
// The screen is a chart of feat chains: a CSWGuiSkillFlowChart of rows with
// up to three cells (the feat, its improved and its master form), embedded as
// the single child of the feats list box. Each cell carries a feat id and a
// status. The keyboard model is the shared one in menus_skillflow_grid.h.

#pragma once

namespace acc::menus::chargen_feats {

// True iff `panel`'s vtable is CSWGuiFeatsCharGen's.
bool IsChargenFeatsPanel(void* panel);

// True when the event is consumed; `outRv` is then the input hook's result.
bool HandleInput(void* panel, int code, int value, int& outRv);

}  // namespace acc::menus::chargen_feats
