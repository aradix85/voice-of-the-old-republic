
#include "peek_description_internal.h"

#include "engine_input.h"
#include "engine_offsets.h"
#include "engine_rebase.h"
#include "hotkeys.h"
#include "log.h"
#include "menus_abilities.h"  // RefreshDetail (abilities description repaint)
#include "menus_crafting.h"   // ResolveRowCommit — K2 crafting row peek gate
#include "menus_galaxymap.h"  // SpeakDescription — galaxy-map LBL_DESC peek
#include "menus_internal.h"   // panel-member control resolvers
#include "menus_listbox.h"    // IsEquipPickerArmed
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

// CSWGuiInGameJournal::OnControlEntered — no is_active gate, direct call.
// (Duplicate of menus_journal.cpp's file-local copy — keep the two values
// in sync. K2 twin 0x007FC390, identified with the journal batch 3d work.)
const std::uintptr_t kAddrJournalOnControlEntered = acc::addr::Pick(0x00645100, 0x007FC390);

static void RefreshInventory(void* panel, void* focused) {
    if (!panel || !focused) return;
    CallOnControlEnteredWithActive(kAddrInventoryOnControlEntered,
                                   panel, focused);
}

static void RefreshStore(void* panel, void* focused) {
    if (!panel || !focused) return;
    CallOnControlEnteredWithActive(kAddrStoreOnControlEntered,
                                   panel, focused);
}

static void RefreshJournal(void* panel, void* focused) {
    if (!panel || !focused) return;
    if (!acc::addr::Ok(kAddrJournalOnControlEntered)) {
        acclog::Write("Peek", "RefreshJournal skipped: address unresolved on build %s",
                      acc::addr::ActiveBuildName());
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
static void RefreshAbilities(void* panel, void* /*focused*/) {
    if (!panel) return;
    acc::menus::abilities::RefreshDetail(panel);
}

// Panel kind → description-listbox offset + optional refresh adapter.
// Adding a panel = one row. refresh=nullptr reads whatever the engine
// already populated.
//
// Journal's slot is named `item_description_label` in SARIF but its
// DATATYPE is CSWGuiListBox — misnamed, treat as listbox.
//
// Options sub-panels also have description_listbox members but
// IdentifyPanel currently lumps them all under InGameOptions —
// settings-tooltip support waits on per-tab kind discrimination.
struct PanelPeekInfo {
    acc::engine::PanelKind kind;
    std::size_t            descListBoxOffset;
    void                 (*refresh)(void* panel, void* focused);  // optional
};

const PanelPeekInfo kPanels[] = {
    // Inventory description_listbox: K1 0x844 (Lane's DB), K2 0x878 — its ctor
    // 0x008A6170 builds two listboxes at +0x588/+0x878 in the same
    // items-then-description order as K1's +0x564/+0x844.
    { acc::engine::PanelKind::InGameInventory,  acc::off::Pick(0x0844, 0x0878), RefreshInventory  },  // CSWGuiInGameInventory.description_listbox
    // Duplicate of menus_journal.cpp's kJournalDescriptionListBoxOffset —
    // keep the two values in sync (same arrangement as the journal
    // OnControlEntered address above).
    { acc::engine::PanelKind::InGameJournal,    acc::off::Pick(0x01a4, 0x01b0), RefreshJournal    },  // CSWGuiInGameJournal.item_description_label (a CSWGuiListBox)
    { acc::engine::PanelKind::InGameAbilities,  kAbilitiesDescListBoxOffset, RefreshAbilities },  // LB_DESC (0x33bc); refresh repaints it for the focused entry
    { acc::engine::PanelKind::Store,            kStoreDescriptionListBoxOffset, RefreshStore      },  // CSWGuiStore.description_listbox
    // CSWGuiInGameEquip is handled via the item-tooltip path below: its
    // arrow-key nav goes through DriveListBoxSelection which bypasses the
    // engine's onSelectionChanged callback, so description_listbox at
    // +0x33b8 stays stale. Instead we derive the focused row from
    // items_listbox.selection_index and read the item description directly.
};

const PanelPeekInfo* LookupPanel(acc::engine::PanelKind k) {
    for (const auto& p : kPanels) {
        if (p.kind == k) return &p;
    }
    return nullptr;
}

bool ShiftHeld() {
    // OS-level query; the engine-side flag can latch stale on swallowed
    // up-edges.
    return acc::hotkeys::ShiftHeld();
}

}  // namespace

// Several OnControlEntered overrides early-out on `entry->is_active != 0`.
// HandleLMouseDown sets that on click; our keyboard chain step doesn't,
// so equipped inventory rows stay at 0 forever. Save → force 1 → call →
// restore around the engine call only.
void CallOnControlEnteredWithActive(std::uintptr_t addr,
                                           void* panel, void* focused)
{
    // Guard before touching is_active: on a rebased build an unmapped address
    // is 0, and we must not force the flag and then fail to make the call that
    // restores it.
    if (!acc::addr::Ok(addr)) {
        acclog::Write("Peek", "OnControlEntered skipped: address unresolved on build %s",
                      acc::addr::ActiveBuildName());
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
    // Drop the item-block cache so the next peek rebuilds for whatever item is
    // focused then (the user may have moved to a different row meanwhile).
    g_blockCacheItem = nullptr;
    g_blockCount = 0;
}

bool HandleShiftArrow(int param_1, int param_2, void* activePanel,
                      void* focusedControl) {
    // Press edge only — release events fall through.
    if (param_2 == 0) return false;
    if (param_1 != kInputNavUp && param_1 != kInputNavDown) return false;
    if (!ShiftHeld()) return false;
    if (!activePanel) return false;

    const bool down = (param_1 == kInputNavDown);
    auto kind = acc::engine::IdentifyPanel(activePanel);

    // Slot path runs before picker-listbox because InGameEquip hosts
    // both surfaces; the picker is empty until a slot is drilled into.
    //
    // Skip the slot path while the picker is armed — chain focus stays
    // on the originating slot button while picker rows are driven by
    // DriveListBoxSelection, so without this gate the slot-index match
    // would hit the slot, find the handle at 0x7f000000 (engine moves
    // the item out for the swap), and silently early-out instead of
    // letting the item-tooltip path read the row the user is sitting on.
    if (kind == acc::engine::PanelKind::InGameEquip && focusedControl &&
        !acc::menus::listbox::IsEquipPickerArmed()) {
        int slotIdx = acc::menus::detail::EquipSlotIndexFromButton(
            activePanel, focusedControl);
        if (slotIdx >= 0) {
            HandleEquipSlotTooltip(activePanel, slotIdx, down);
            return true;
        }
    }

    // Workbench upgrade slot button (matched by position in the panel's
    // embedded button run — UpgradeSlotIndexFromButton): speak the
    // installed mod's description, or "empty". Runs before the item-tooltip
    // path because that path targets LB_ITEMS — the mod picker shown only
    // after a slot is drilled into.
    //
    // Skip while the picker is armed — exactly like the InGameEquip case
    // above: once a slot is drilled into, chain focus STAYS on the slot
    // button while the offered-mod rows are driven by DriveListBoxSelection
    // (verified from the patch log: Shift+arrow kept hitting the slot button
    // with the picker open). Without this gate we'd keep reading the slot's
    // installed mod instead of letting the item-tooltip path read the
    // offered crystal the user is actually sitting on.
    if (kind == acc::engine::PanelKind::WorkbenchUpgrade && focusedControl &&
        !acc::menus::listbox::IsWorkbenchUpgradePickerArmed()) {
        if (acc::menus::detail::UpgradeSlotIndexFromButton(
                activePanel, focusedControl) >= 0) {
            HandleWorkbenchSlotTooltip(activePanel, focusedControl, down);
            return true;
        }
    }

    // Galaxy / star-map travel screen: read LBL_DESC for the currently
    // displayed planet. The planet itself is cycled with plain Up/Down
    // (galaxymap::TryHandleInput); Shift+Down is the description peek.
    if (kind == acc::engine::PanelKind::InGameGalaxyMap) {
        acc::menus::galaxymap::SpeakDescription(activePanel);
        return true;
    }

    // Item-tooltip path (Container, Equip picker, Workbench listboxes).
    if (const ItemTooltipPanelInfo* itemInfo = LookupItemTooltipPanel(kind)) {
        HandleItemTooltip(kind, *itemInfo, activePanel, down);
        return true;
    }

    // Inventory / Store / QuestItems: the chain-focused control is the item
    // entry row itself (client item handle at +0x1c4). Read the item and
    // navigate its four categorised blocks, instead of paging the single-row
    // description listbox the engine renders. Falls through to the listbox
    // path only if the focused row doesn't resolve to an item (e.g. focus
    // parked on a non-item control).
    //
    // All three are chain-navigated and all three build their rows from the
    // same CSWGuiInGameItemEntry class, so the chain entry is the only thing
    // that knows where the user is — see the membership rule on
    // kItemTooltipPanels.
    if (kind == acc::engine::PanelKind::InGameInventory ||
        kind == acc::engine::PanelKind::InGameQuestItems ||
        kind == acc::engine::PanelKind::Store) {
        if (void* item = ResolveRowItem(focusedControl)) {
            acclog::Write("Peek.Blocks",
                          "panel=%s focused row item=%p -> block nav",
                          acc::engine::PanelKindName(kind), item);
            if (SpeakItemBlocks(item, down)) return true;
        }
    }

    // K2 workbench screens (upgradesel item list, create/breakdown lists):
    // same chain-focused row model as the trio above, but gated on actual
    // row membership first — the panels' category/action BUTTONS are only
    // 0x1d0 bytes, so ResolveRowItem's id read on one would pull garbage
    // from past the control. ResolveRowCommit does the membership scan.
    // Create-list rows are panel-local template items whose handle may not
    // resolve; those fall through silently to the generic tooltip path.
    {
        void* peekLb = nullptr;
        if (acc::menus::crafting::ResolveRowCommit(activePanel, focusedControl,
                                                   &peekLb)) {
            if (void* item = ResolveRowItem(focusedControl)) {
                acclog::Write("Peek.Blocks",
                              "panel=%s crafting row item=%p -> block nav",
                              acc::engine::PanelKindName(kind), item);
                if (SpeakItemBlocks(item, down)) return true;
            }
        }
    }

    const PanelPeekInfo* info = LookupPanel(kind);
    if (!info) {
        // Generic fallback: focused control's own .gui tooltip strref.
        // Covers LevelUp / PartySelection / options widgets / etc.
        if (focusedControl) {
            char tip[8192];
            if (acc::engine::ReadControlTooltip(focusedControl, tip,
                                                sizeof(tip))) {
                acc::speech::Speak(tip, /*interrupt=*/true);
                acclog::Write("Peek.Control",
                              "panel=%s control=%p tooltip=\"%s\"",
                              acc::engine::PanelKindName(kind),
                              focusedControl, tip);
                return true;
            }
            acclog::Write("Peek.Control",
                          "panel=%s control=%p no tooltip; pass through",
                          acc::engine::PanelKindName(kind), focusedControl);
        }
        return false;  // unknown / no tooltip — pass through to plain nav
    }

    // Re-stage description for the focused row. SEH-guarded against
    // stale entries during panel teardown.
    if (info->refresh) {
        __try {
            info->refresh(activePanel, focusedControl);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            acclog::Write("Peek", "panel=%s refresh SEH (focused=%p); "
                          "reading stale rows",
                          acc::engine::PanelKindName(kind), focusedControl);
        }
    }

    // Inline at known offset; address is the listbox base, no lookup.
    void* lb = reinterpret_cast<unsigned char*>(activePanel) +
               info->descListBoxOffset;

    auto* lbList = reinterpret_cast<CExoArrayList*>(
        reinterpret_cast<unsigned char*>(lb) + kListBoxControlsOffset);
    int rowCount = (lbList && lbList->data) ? lbList->size : 0;

    if (rowCount <= 0) {
        // Consume the key anyway (predictable-behaviour rule).
        acclog::Write("Peek", "panel=%s lb=%p rowCount=0; silent",
                      acc::engine::PanelKindName(kind), lb);
        return true;
    }

    int prev = g_blockIdx;
    if (g_blockIdx == kCursorReset) {
        // First press after a release speaks block 0 in either direction.
        g_blockIdx = 0;
    } else if (down) {
        if (g_blockIdx < rowCount - 1) ++g_blockIdx;
    } else {
        if (g_blockIdx > 0) --g_blockIdx;
    }

    // Clamp — panel may have re-populated with fewer rows.
    if (g_blockIdx >= rowCount) g_blockIdx = rowCount - 1;

    void* row = lbList->data[g_blockIdx];
    char text[8192];
    const char* src = row ? ReadRowText(row, text, sizeof(text)) : nullptr;
    if (src) {
        // interrupt=true: each press is a discrete read; fast double-
        // press should land on the latest block, not queue both.
        acc::speech::Speak(text, /*interrupt=*/true);
        acclog::Write("Peek", "panel=%s block %d/%d (was %d) src=%s text=\"%s\"",
                      acc::engine::PanelKindName(kind),
                      g_blockIdx, rowCount, prev, src, text);
    } else {
        acclog::Write("Peek", "panel=%s block %d/%d (was %d) row=%p no-text; silent",
                      acc::engine::PanelKindName(kind),
                      g_blockIdx, rowCount, prev, row);
    }

    return true;
}

}  // namespace acc::peek
