// The chain state, defined in one place; each item is described where it is
// declared, in menus_chain.h (g_chainVtable in menus_chain_internal.h).

#include "menus_chain_internal.h"

namespace acc::menus::chain {

ChainEntry g_chain[kMaxChainEntries];
void*      g_chainPanel  = nullptr;
uintptr_t  g_chainVtable = 0;
int        g_chainIndex  = 0;
int        g_chainCount  = 0;
void* g_tabbedPanel = nullptr;
int   g_tabsStart   = -1;
int   g_tabsCount   = 0;
int g_equipSlotClickOffsetY  = 0;
int g_classIconClickOffsetX  = 0;

}  // namespace acc::menus::chain
