// Internal to the menus_monitors module (menus_monitors*.cpp); no other file includes
// it. The public interface is menus_monitors.h. Here: what the files share.

#pragma once

#include "menus_monitors.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_manager.h"
#include "engine_panels.h"
#include "log.h"
#include "menus_internal.h"

using namespace acc::engine;

namespace acc::menus::monitors {

// Helpers shared across the files, by the file that defines them.

// menus_monitors_subscreens.cpp
void AnnounceNewSubScreens(void** panels, int count);

// menus_monitors_content.cpp
void MonitorPanelContents();

// menus_monitors_dialog.cpp
void MonitorDialogReplies();

// menus_monitors_inventory.cpp
void MonitorInventoryListPopulation();

}  // namespace acc::menus::monitors
