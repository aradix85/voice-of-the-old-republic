// Shared between the menus_crafting files: the workbench panel kinds, the
// visible craft list, and the latch that tells Tick the user opened the
// upgrade screen from the list.

#pragma once

#include "engine_panels.h"
#include "menus_crafting.h"

namespace acc::menus::crafting {

bool IsCraftingKind(acc::engine::PanelKind k);
bool IsWorkbenchFamilyKind(acc::engine::PanelKind k);
bool IsListBoxVisible(void* listBox);
void* FindActiveCraftList(void* panel);

extern int g_userOpenedUpgradeTtl;

}  // namespace acc::menus::crafting
