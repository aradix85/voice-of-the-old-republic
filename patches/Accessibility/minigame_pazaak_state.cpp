// The state the minigame_pazaak files share, defined in one place; each item is
// described where it is declared, in minigame_pazaak_internal.h.

#include "minigame_pazaak_internal.h"

namespace acc::pazaak {

Snap      g_prev;
void*     g_panel = nullptr;
bool      g_started = false;
bool      g_resultAnnounced = false;
bool      g_boardForeground = false;
int  g_zone = kZoneHand;
int  g_col  = 0;
bool g_optMode   = false;
int  g_optSlot   = -1;
int  g_optSign   = 0;
int  g_optValue  = 1;

}  // namespace acc::pazaak
