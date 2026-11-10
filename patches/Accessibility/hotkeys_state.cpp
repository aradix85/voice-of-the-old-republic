// The state the hotkeys files share, defined in one place; each item is
// described where it is declared, in hotkeys_internal.h.

#include "hotkeys_internal.h"

namespace acc::hotkeys {

Binding g_bindings[static_cast<int>(Action::COUNT)] = {};
Binding g_defaults[static_cast<int>(Action::COUNT)] = {};

}  // namespace acc::hotkeys
