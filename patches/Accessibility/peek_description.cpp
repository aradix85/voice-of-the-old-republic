// Shift+Up/Down description peek: the panel descriptions (inventory,
// journal, abilities, store, galaxy map, any control's tooltip) and the
// dispatch to the item-description paths in peek_description_item.cpp.

#include "peek_description_internal.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_addr.h"
#include "hotkeys.h"
#include "log.h"
#include "menus_abilities.h"
#include "menus_crafting.h"
#include "menus_galaxymap.h"
#include "menus_internal.h"
#include "menus_listbox.h"
#include "speech.h"

namespace acc::peek {

namespace {

typedef void (__thiscall* PFN_PanelOnControl)(void* panel, void* control);

// CSWGuiInGameInventory::OnControlEntered — is_active gate applies.
// K2 twin 0x008A8100, decompile-matched: same is_active gate ([control+0x50]),
// item id at [row+0x1d0], the identical "Error: Invalid item" literal, and the
// same desc-label restage tail. Registered for event 0 (control-entered) by
// the K2 CreateItemEntry 0x008A75F0 — the registration route that found it.
const std::uintptr_t kAddrInventoryOnControlEntered = acc::addr::Pick(0x006b3d10, 0x008A8100);

// CSWGuiStore::OnControlEntered — same is_active gate as Inventory.
// K2 twin 0x008B6E90, decompile-matched: is_active gate, row id [row+0x1d0]
// through ClientToServerObjectId 0x0051C8B0, buy/sell value twins
// 0x008B8000/0x008B8070, and K1's exact 0xa3df unlimited-stock strref.
const std::uintptr_t kAddrStoreOnControlEntered = acc::addr::Pick(0x006c0aa0, 0x008B6E90);

void RefreshInventory(void* panel, void* focused) {
    if (!panel || !focused) return;
    CallOnControlEnteredWithActive(kAddrInventoryOnControlEntered,
                                   panel, focused);
}

void RefreshStore(void* panel, void* focused) {
    if (!panel || !focused) return;
    CallOnControlEnteredWithActive(kAddrStoreOnControlEntered,
                                   panel, focused);
}

void RefreshJournal(void* panel, void* focused) {
    if (!panel || !focused) return;
    if (!acc::addr::Ok(kAddrJournalOnControlEntered)) {
        acclog::Write("Peek", "RefreshJournal skipped: address unresolved on build %s",
                      acc::game::TitleName());
        return;
    }
    auto fn = reinterpret_cast<PFN_PanelOnControl>(
        kAddrJournalOnControlEntered);
    fn(panel, focused);
}

// Repaint LB_DESC for the focused entry before Shift+Up/Down pages it. Routes
// through the abilities module's OnEnterSkill path (NOT OnAbilitySelectionChanged,
// which is mouse-coordinate-driven and corrupts the stack via a ret-4 mismatch).
// `focused` is unused — the selection lives on the ability_listbox.
void RefreshAbilities(void* panel, void* /*focused*/) {
    if (!panel) return;
    acc::menus::abilities::RefreshDetail(panel);
}

// Panel kind -> description-listbox offset + optional refresh adapter;
// refresh=nullptr reads whatever the engine already populated. Equip is not
// here: DriveListBoxSelection bypasses its onSelectionChanged, so its
// description_listbox stays stale and it goes through the item-tooltip path.
struct PanelPeekInfo {
    acc::engine::PanelKind kind;
    std::size_t            descListBoxOffset;
    void                 (*refresh)(void* panel, void* focused);  // optional
};

const PanelPeekInfo kPanels[] = {
    // Inventory description_listbox: K2's ctor 0x008A6170 builds the items and
    // description listboxes at +0x588/+0x878, in K1's +0x564/+0x844 order.
    { acc::engine::PanelKind::InGameInventory, acc::off::Pick(0x0844, 0x0878),
      RefreshInventory },
    { acc::engine::PanelKind::InGameJournal, kJournalDescriptionListBoxOffset, RefreshJournal },
    { acc::engine::PanelKind::InGameAbilities, kAbilitiesDescListBoxOffset, RefreshAbilities },
    { acc::engine::PanelKind::Store, kStoreDescriptionListBoxOffset, RefreshStore },
};

const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k) {
    for (const auto& p : kPanels) {
        if (p.kind == k) return &p;
    }
    return nullptr;
}

}  // namespace

int g_blockIdx = kCursorReset;

int StepBlockCursor(bool down, int count) {
    const int prev = g_blockIdx;
    if (g_blockIdx == kCursorReset) {
        g_blockIdx = 0;  // the first press speaks block 0 in either direction
    } else if (down) {
        if (g_blockIdx < count - 1) ++g_blockIdx;
    } else {
        if (g_blockIdx > 0) --g_blockIdx;
    }
    if (g_blockIdx >= count) g_blockIdx = count - 1;  // the list may have shrunk
    return prev;
}

// Several OnControlEntered overrides early-out on `entry->is_active != 0`.
// HandleLMouseDown sets that on click; our keyboard chain step doesn't,
// so equipped inventory rows stay at 0 forever. Save → force 1 → call →
// restore around the engine call only.
void CallOnControlEnteredWithActive(std::uintptr_t addr, void* panel, void* focused) {
    // Guard before touching is_active: an address not known for this game
    // is 0, and we must not force the flag and then fail to make the call that
    // restores it.
    if (!acc::addr::Ok(addr)) {
        acclog::Write("Peek", "OnControlEntered skipped: address unresolved on build %s",
                      acc::game::TitleName());
        return;
    }

    auto* isActivePtr = reinterpret_cast<std::uint32_t*>(
        reinterpret_cast<unsigned char*>(focused) + kControlIsActiveOffset);
    std::uint32_t saved = *isActivePtr;
    if (saved == 0) *isActivePtr = 1;

    auto fn = reinterpret_cast<PFN_PanelOnControl>(addr);
    fn(panel, focused);

    *isActivePtr = saved;
}

// Rows can be CSWGuiLabel OR CSWGuiButton (engine uses both for text-only
// containers). Try every known text path. Returns source tag for logging.
const char* ReadRowText(void* row, char* outBuf, std::size_t bufSize) {
    if (!row || !outBuf || bufSize < 2) return nullptr;

    if (acc::engine::ReadGuiString(row, kLabelGuiStringPtrOffset,
                                   outBuf, bufSize)) {
        return "label-gui";
    }
    if (acc::engine::ReadGuiString(row, kButtonGuiStringPtrOffset,
                                   outBuf, bufSize)) {
        return "button-gui";
    }
    if (acc::engine::ExtractTextOrStrRefIndirect(
            row, kLabelTextOffset, kLabelStrRefOffset,
            kLabelTextObjectOffset, outBuf, bufSize)) {
        return "label-cexo";
    }
    if (acc::engine::ExtractTextOrStrRefIndirect(
            row, kButtonTextOffset, kButtonStrRefOffset,
            kButtonTextObjectOffset, outBuf, bufSize)) {
        return "button-cexo";
    }
    return nullptr;
}

void OnShiftReleased() {
    if (g_blockIdx != kCursorReset) {
        acclog::Write("Peek", "shift release; reset block cursor (was %d)",
                      g_blockIdx);
    }
    g_blockIdx = kCursorReset;
    // The next peek rebuilds for whatever item is focused then.
    ResetItemBlocks();
}

namespace {

// Chain-navigated rows whose focused control is the item entry itself. The K2
// workbench lists are checked for row membership first: their category and
// action buttons are only 0x1d0 bytes, so ResolveRowItem's id read on one
// would pull garbage from past the control.
bool IsChainItemRow(acc::engine::PanelKind kind, void* panel, void* focused) {
    if (kind == acc::engine::PanelKind::InGameInventory ||
        kind == acc::engine::PanelKind::InGameQuestItems ||
        kind == acc::engine::PanelKind::Store) {
        return true;
    }
    void* lb = nullptr;
    return acc::menus::crafting::ResolveRowCommit(panel, focused, &lb);
}

// Fallback for panels without a description listbox: the focused control's
// own .gui tooltip (LevelUp, PartySelection, options widgets and the like).
bool SpeakControlTooltip(acc::engine::PanelKind kind, void* focused) {
    if (!focused) return false;
    char tip[8192];
    if (!acc::engine::ReadControlTooltip(focused, tip, sizeof(tip))) {
        acclog::Write("Peek.Control", "panel=%s control=%p no tooltip; pass through",
                      acc::engine::PanelKindName(kind), focused);
        return false;
    }
    acc::speech::Speak(tip, /*interrupt=*/true);
    acclog::Write("Peek.Control", "panel=%s control=%p tooltip=\"%s\"",
                  acc::engine::PanelKindName(kind), focused, tip);
    return true;
}

// Re-stage the panel's description for the focused row, then page its rows.
void PageDescriptionListBox(acc::engine::PanelKind kind, const PanelPeekInfo& info,
                            void* panel, void* focused, bool down) {
    if (info.refresh) {
        __try {
            info.refresh(panel, focused);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            acclog::Write("Peek", "panel=%s refresh SEH (focused=%p); reading stale rows",
                          acc::engine::PanelKindName(kind), focused);
        }
    }

    void* lb = reinterpret_cast<unsigned char*>(panel) + info.descListBoxOffset;
    auto* lbList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(lb) + kListBoxControlsOffset);
    const int rowCount = (lbList && lbList->data) ? lbList->size : 0;
    if (rowCount <= 0) {
        acclog::Write("Peek", "panel=%s lb=%p rowCount=0; silent",
                      acc::engine::PanelKindName(kind), lb);
        return;
    }

    const int prev = StepBlockCursor(down, rowCount);
    void* row = lbList->data[g_blockIdx];
    char text[8192];
    const char* src = row ? ReadRowText(row, text, sizeof(text)) : nullptr;
    if (src) {
        acc::speech::Speak(text, /*interrupt=*/true);
        acclog::Write("Peek", "panel=%s block %d/%d (was %d) src=%s text=\"%s\"",
                      acc::engine::PanelKindName(kind), g_blockIdx, rowCount, prev, src,
                      text);
    } else {
        acclog::Write("Peek", "panel=%s block %d/%d (was %d) row=%p no-text; silent",
                      acc::engine::PanelKindName(kind), g_blockIdx, rowCount, prev, row);
    }
}

}  // namespace

bool HandleShiftArrow(int param_1, int param_2, void* activePanel,
                      void* focusedControl) {
    if (param_2 == 0) return false;  // press edge only
    if (param_1 != kInputNavUp && param_1 != kInputNavDown) return false;
    if (!acc::hotkeys::ShiftHeld()) return false;
    if (!activePanel) return false;

    const bool down = (param_1 == kInputNavDown);
    const auto kind = acc::engine::IdentifyPanel(activePanel);

    // Equip and Workbench-upgrade slot buttons speak the installed item. While
    // the panel's picker is armed, chain focus stays on the slot button but the
    // user is on a picker row (driven by DriveListBoxSelection), so the slot
    // path is skipped and the item-tooltip path below reads that row.
    if (kind == acc::engine::PanelKind::InGameEquip && focusedControl &&
        !acc::menus::listbox::IsEquipPickerArmed()) {
        int slotIdx = acc::menus::detail::EquipSlotIndexFromButton(
            activePanel, focusedControl);
        if (slotIdx >= 0) {
            HandleEquipSlotTooltip(activePanel, slotIdx, down);
            return true;
        }
    }

    if (kind == acc::engine::PanelKind::WorkbenchUpgrade && focusedControl &&
        !acc::menus::listbox::IsWorkbenchUpgradePickerArmed()) {
        if (acc::menus::detail::UpgradeSlotIndexFromButton(
                activePanel, focusedControl) >= 0) {
            HandleWorkbenchSlotTooltip(activePanel, focusedControl, down);
            return true;
        }
    }

    // Plain Up/Down cycles the planets; Shift peeks the shown planet's LBL_DESC.
    if (kind == acc::engine::PanelKind::InGameGalaxyMap) {
        acc::menus::galaxymap::SpeakDescription(activePanel);
        return true;
    }

    if (const ItemTooltipPanelInfo* itemInfo = LookupItemTooltipPanel(kind)) {
        HandleItemTooltip(kind, *itemInfo, activePanel, down);
        return true;
    }

    // Walk the focused item's categorised blocks instead of the single-row
    // description the engine renders; a row without an item falls through.
    if (IsChainItemRow(kind, activePanel, focusedControl)) {
        if (void* item = ResolveRowItem(focusedControl)) {
            acclog::Write("Peek.Blocks", "panel=%s row item=%p -> block nav",
                          acc::engine::PanelKindName(kind), item);
            if (SpeakItemBlocks(item, down)) return true;
        }
    }

    const PanelPeekInfo* info = LookupPanel(kind);
    if (!info) return SpeakControlTooltip(kind, focusedControl);
    PageDescriptionListBox(kind, *info, activePanel, focusedControl, down);
    return true;  // consumed even when silent
}

}  // namespace acc::peek
