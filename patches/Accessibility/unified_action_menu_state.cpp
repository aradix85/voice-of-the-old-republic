// The state the unified_action_menu files share, defined in one place; each item is
// described where it is declared, in unified_action_menu_internal.h.

#include "unified_action_menu_internal.h"

namespace acc::unified_menu {

int g_targetSel[kRowCount]      = {0, 0, 0};
int g_personalSel[kColumnCount] = {0, 0, 0, 0, 0, 0};
State g;

}  // namespace acc::unified_menu
