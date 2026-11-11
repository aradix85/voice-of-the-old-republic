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

// The Shift+Up/Down cursor over listbox rows or item blocks; reset on Shift release.
extern int g_blockIdx;

// Item-tooltip path for listbox-driven panels with no inline description
// listbox (Container, Equip picker), or where OnItemSelected would commit the
// action. findLb is the panel's member resolver; minSel=1 skips the Equip
// picker's template row.
struct ItemTooltipPanelInfo {
    acc::engine::PanelKind kind;
    void*                (*findLb)(void* panel);
    int                    minSel;
};

// Helpers shared across the files, by the file that defines them.

// peek_description.cpp
int StepBlockCursor(bool down, int count);  // returns the previous cursor
void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused);
const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize);

// peek_description_item.cpp
void ResetItemBlocks();
bool SpeakItemBlocks(void* item, bool down);
bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down);
bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down);
const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k);
bool HandleItemTooltip(acc::engine::PanelKind kind, const ItemTooltipPanelInfo& info,
                       void* activePanel, bool down);

}  // namespace acc::peek
