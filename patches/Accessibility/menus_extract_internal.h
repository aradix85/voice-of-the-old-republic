// Internal to the menus_extract module (menus_extract*.cpp); no other file includes
// it. The public interface is menus_extract.h. Here: what the files share.

#pragma once

#include "menus_extract.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_area.h"        // GetObjectDisplayNameByHandle
#include "engine_game.h"        // IsKotor2 — K1-only portraits.2da row table
#include "engine_manager.h"
#include "engine_offsets.h"
#include "engine_panels.h"
#include "engine_player.h"      // PartyTableIsNPCAvailable / *Selectable /
                                // GetPartyNpcNameForSlot
#include "engine_reads.h"
#include "log.h"
#include "menus_charsheet.h"
#include "menus_credits.h"
#include "menus_listbox.h"      // IsEquipPickerArmed / IsWorkbenchUpgradePickerArmed
                                // — an armed picker clears the panel buttons'
                                // interactive bit; see AppendDisabledSuffix
#include "menus_equipstats.h"
#include "menus_pazaakdeck.h"
#include "menus_internal.h"
#include "menus_modsettings.h"
#include "strfmt.h"   // AsciiLower
#include "strings.h"
#include "engine_rebase.h"
#include "engine_offsets_select.h"

using namespace acc::engine;

// Bring shared seam helpers into unqualified scope so the function bodies
// read as they did when everything lived in menus.cpp.
using acc::menus::detail::IsChainNavigable;
using acc::menus::detail::IsClassSelectionIcon;
using acc::menus::detail::ClassLabelCacheLookup;
using acc::menus::detail::ClassLabelCacheStore;
using acc::menus::detail::GetControlCenter;
using acc::menus::detail::ScaleGuiThresholdPx;
using acc::menus::detail::kAnnounceTextMax;

namespace acc::menus::extract {

// Helpers shared across the files, by the file that defines them.

// menus_extract.cpp
bool EmitText(char* outBuf, size_t bufSize, const char* text);
void* ResolveOwnerPanel(void* control, void* ownerPanel);
bool IsPanelDirectChild(void* panel, void* control);

// menus_extract_controls.cpp
const char* TryTooltip(void* control, char* outBuf, size_t bufSize);
const char* TryButton(void* control, char* outBuf, size_t bufSize);
const char* TryButtonToggle(void* control, char* outBuf, size_t bufSize);
const char* TryLabel(void* control, char* outBuf, size_t bufSize);
const char* TryLabelHilight(void* control, char* outBuf, size_t bufSize);
const char* TrySlider(void* control, void* owner, char* outBuf, size_t bufSize);
const char* TryEditbox(void* control, char* outBuf, size_t bufSize);
const char* TrySingleRowListBox(void* control, char* outBuf, size_t bufSize);
const char* TryItemEntryRow(void* control, char* outBuf, size_t bufSize);
const char* TrySiblingLabel(void* control, void* owner, char* outBuf, size_t bufSize);

// menus_extract_rows.cpp
const char* TryVirtualRowAnchor(void* control, void* ownerPanel, char* outBuf, size_t bufSize);

// menus_extract_icons.cpp
const char* TryPartyPortrait(void* control, char* outBuf, size_t bufSize, bool& claimedVtable);
const char* TrySpeculativeVtableRead(void* control, char* outBuf, size_t bufSize);
const char* TryInGameMenuIcon(void* control, void* owner, char* outBuf, size_t bufSize);
const char* TryEquipSlot(void* control, void* owner, char* outBuf, size_t bufSize);
const char* TryInGameMapArrow(void* control, void* owner, char* outBuf, size_t bufSize);

// menus_extract_workbench.cpp
const char* TryWorkbenchSlot(void* control, void* owner, char* outBuf, size_t bufSize);
const char* TryWorkbenchCreateItemsButton(void* control, void* owner, char* outBuf, size_t bufSize);

// menus_extract_chargen.cpp
const char* TryClassSelectionIcon(void* control, void* owner, char* outBuf, size_t bufSize);
const char* TryPortraitCharGenArrow(void* control, void* owner, char* outBuf, size_t bufSize);

}  // namespace acc::menus::extract
