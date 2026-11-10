// Internal to the peek_description module (peek_description*.cpp); no other file includes
// it. The public interface is peek_description.h. Here: what the files share.

#pragma once

#include "peek_description.h"

#include "engine_panels.h"
#include "engine_reads.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace acc::peek {

constexpr int kCursorReset = -1;

extern int g_blockIdx;

// Item-block navigator state. While Shift is held, Shift+Up/Down moves over the
// non-empty categorised blocks (Tags / Values / Properties / Description) of the
// focused item; the cache is rebuilt when the focused item changes and cleared
// on Shift release (OnShiftReleased). g_blockIdx (above) is the shared cursor.
extern void* g_blockCacheItem;
extern acc::engine::ItemDescriptionBlocks g_blocks;
extern const char* g_blockPtrs[4];
extern int g_blockCount;

// Item-tooltip path for listbox-driven panels with no inline
// description_listbox (Container, Equip picker) or where calling
// OnItemSelected would commit the action (Equip — picker resolves on
// the user's actual click). Resolves CSWGuiInGameItemEntry rows
// (item_game_object_id at +0x1c4) via the same client→server handle
// path the engine uses, then reads CSWSItem::GetPropertyDescription.
//
// minSel=1 skips the Equip-picker's protoitem template row;
// Container uses 0.
//
// findLb: the panel's ctor-bound member resolver — every entry now names an
// engine member rather than a .gui id.
struct ItemTooltipPanelInfo {
    acc::engine::PanelKind kind;
    void*                (*findLb)(void* panel);
    int                    minSel;
};

// Helpers shared across the files, by the file that defines them.

// peek_description.cpp
void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused);
const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize);

// peek_description_item.cpp
bool SpeakItemBlocks(void* item, bool down);
bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down);
bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down);
const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k);
void* ResolveRowItem(void* row);
bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info, void* activePanel, bool down);

}  // namespace acc::peek
