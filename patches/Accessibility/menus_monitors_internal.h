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

using acc::menus::detail::IsClassSelectionIcon;
using acc::menus::detail::FindListBoxChild;

namespace acc::menus::monitors {

// Spec order = the icon strip's left-to-right visual order, which is what
// Tab / Shift+Tab cycle through. `guiId` is the engine's own sub-screen
// index used by CGuiInGame::SwitchToSWInGameGui — not the same as strip
// position (CGuiInGame allocates its slots in 0x0c..0x28 order, which
// puts Map and Messages in the "wrong" spots from a strip POV). Keeping
// both in one row lets the Tab handler look up the next strip neighbour
// and the engine GUI_id in a single pass.
// strrefK1/strrefK2: KOTOR 2's dialog.tlk assigns KOTOR 1's ids to unrelated
// strings (48218 there is "Ablativplatten Mk 1"), so each game needs its own
// cluster — K1 48218..48225, K2 48620..48628. Same values as the icon table in
// menus_extract.cpp (slot 4 there matches the PartySelection row on KOTOR 2
// and the InGameMessages row on KOTOR 1); keep the two in sync. Use
// SpecStrref() to read them.
// kotor2Only: the row is a sub-screen on KOTOR 2 alone; FindSpec skips it on
// KOTOR 1. Rows that leave it out get false.
struct InGameSubScreenSpec {
    PanelKind   kind;
    int         guiId;
    uint32_t    strrefK1;
    uint32_t    strrefK2;
    const char* literal;
    bool        kotor2Only;
};

// Helpers shared across the files, by the file that defines them.

// menus_monitors_subscreens.cpp
const InGameSubScreenSpec* FindSpec(PanelKind k);
void AnnounceNewSubScreens(void** panels, int count);

// menus_monitors_content.cpp
void MonitorPanelContents();

// menus_monitors_dialog.cpp
void MonitorDialogReplies();

// menus_monitors_inventory.cpp
void MonitorInventoryListPopulation();

}  // namespace acc::menus::monitors
