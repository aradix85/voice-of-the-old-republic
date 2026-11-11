// Reading an item's description block by block: the equipment and workbench slot tooltips, the item
// rows of the item screens, and the workbench saber description.

#include "peek_description_internal.h"

#include "engine_area.h"      // kItemLocNameOffset
#include "engine_offsets.h"
#include "engine_reads.h"
#include "log.h"
#include "menus_abilities.h"  // RefreshDetail (abilities description repaint)
#include "menus_internal.h"   // panel-member control resolvers
#include "speech.h"
#include "strings.h"          // WorkbenchSlotPeekEmpty

namespace acc::peek {

namespace {

// Defined below (uses ReadRowText). Builds the workbench picker description
// via the engine's own hover handler for saber upgrades.
bool TryReadWorkbenchSaberDescription(void* panel, void* row,
                                      char* outBuf, std::size_t bufSize);

bool IsBlank(const char* s) {
    for (; s && *s; ++s) {
        if (!std::isspace(static_cast<unsigned char>(*s))) return false;
    }
    return true;
}

// Copy src into dst with leading/trailing whitespace stripped (the engine
// builders pad each block with a trailing "\n\n").
void TrimCopy(char* dst, std::size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    while (*src && std::isspace(static_cast<unsigned char>(*src))) ++src;
    std::size_t n = std::strlen(src);
    while (n > 0 && std::isspace(static_cast<unsigned char>(src[n - 1]))) --n;
    if (n >= cap) n = cap - 1;
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

// Container / equip / workbench-upgrade listboxes all resolve through the
// shared engine-member resolvers (menus_internal.cpp) — the same
// ctor-mined offsets this file used to duplicate locally
// (docs/gui-id-audit.md). The Container one carried its own copy of the
// K1 +0x7f0 / K2 +0x824 offset until the container batch confirmed both
// values straight out of the two ctors.
void* ContainerFindLb(void* panel) {
    return acc::menus::detail::ContainerPanelItemsListBox(panel);
}

void* InGameEquipFindLb(void* panel) {
    return acc::menus::detail::EquipPanelItemsListBox(panel);
}

void* WorkbenchItemsFindLb(void* panel) {
    return acc::menus::detail::WorkbenchItemsPanelItemsListBox(panel);
}

void* WorkbenchUpgradeFindLb(void* panel) {
    return acc::menus::detail::UpgradePanelItemsListBox(panel);
}

// Workbench saber-upgrade picker: build the description the way the engine's
// own hover handler (CSWGuiUpgrade::OnControlEntered) does, so the keyed bonus
// line it prepends — which the generic GetPropertyDescription path drops for
// crystals (item type 46) — is spoken too. Exact parity with the on-screen
// text. Saber category only (panel.field25 == 1); returns false for non-saber
// items so the caller keeps GetPropertyDescription (richer there: it carries
// those mods' full property block, which the workbench picker also omits).
bool TryReadWorkbenchSaberDescription(void* panel, void* row,
                                      char* outBuf, std::size_t bufSize) {
    if (!panel || !row || !outBuf || bufSize < 2) return false;

    // field25 is a 1-byte category (0/1/2); read it as uint8_t to match the
    // proven-correct menus_extract.cpp read. A 4-byte int read here pulls in
    // neighbouring panel bytes and mis-compares against 1 when they're non-zero.
    uint8_t category = 0;
    if (!acc::engine::TryReadU8(panel, kUpgradePanelCategoryOff, &category)) return false;
    if (category != 1) {  // non-saber → caller uses GetPropertyDescription
        acclog::Write("Peek.Workbench",
                      "panel=%p row=%p category=%u (!=1); non-saber path",
                      panel, row, (unsigned)category);
        return false;
    }

    // Drive the engine's hover handler; SetDescription then writes the combined
    // keyed-property + description string into the panel's description label.
    // The is_active gate applies (keyboard nav never sets it), same workaround
    // as the Inventory/Store refresh path.
    __try {
        CallOnControlEnteredWithActive(kAddrCSWGuiUpgradeOnControlEntered,
                                       panel, row);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Peek.Workbench",
                      "panel=%p row=%p OnControlEntered SEH", panel, row);
        return false;
    }

    void* descLabel = reinterpret_cast<unsigned char*>(panel) +
                      kUpgradeDescLabelOffset;
    const char* src = ReadRowText(descLabel, outBuf, bufSize);
    if (src == nullptr || outBuf[0] == '\0') {
        acclog::Write("Peek.Workbench",
                      "panel=%p row=%p desc label empty (src=%s); fall through",
                      panel, row, src ? src : "null");
        return false;
    }
    return true;
}

// The focused item's non-empty description blocks (Tags / Values /
// Properties / Description), rebuilt when the item changes and dropped on
// Shift release.
void* g_blockCacheItem = nullptr;
acc::engine::ItemDescriptionBlocks g_blocks;
const char* g_blockPtrs[4] = {nullptr, nullptr, nullptr, nullptr};
int g_blockCount = 0;

}  // namespace

void ResetItemBlocks() {
    g_blockCacheItem = nullptr;
    g_blockCount = 0;
}

// Shift+Up/Down over an item's four categorised description blocks. Rebuilds the
// cache when `item` changes, advances the shared cursor (clamped at both ends —
// the re-spoken boundary block is the "end of list" cue), and speaks the current
// block trimmed. Empty categories are dropped at build time, so navigation only
// visits blocks that have content. Returns true when a block was spoken (caller
// still consumes the key on the no-content path for predictable behaviour).
bool SpeakItemBlocks(void* item, bool down) {
    if (!item) return false;

    if (item != g_blockCacheItem) {
        if (!acc::engine::BuildItemDescriptionBlocks(item, &g_blocks)) {
            g_blockCacheItem = nullptr;
            g_blockCount = 0;
            acclog::Write("Peek.Blocks", "item=%p build failed; silent", item);
            return false;
        }
        g_blockCacheItem = item;
        g_blockCount = 0;
        const char* all[4] = {g_blocks.tags, g_blocks.values,
                              g_blocks.properties, g_blocks.description};
        for (int i = 0; i < 4; ++i) {
            if (!IsBlank(all[i])) g_blockPtrs[g_blockCount++] = all[i];
        }
        g_blockIdx = kCursorReset;  // fresh navigation for the new item
    }

    if (g_blockCount == 0) {
        acclog::Write("Peek.Blocks", "item=%p no non-empty blocks; silent",
                      item);
        return false;
    }

    const int prev = StepBlockCursor(down, g_blockCount);

    char spoken[4096];
    TrimCopy(spoken, sizeof(spoken), g_blockPtrs[g_blockIdx]);
    acc::speech::Speak(spoken, /*interrupt=*/true);
    acclog::Write("Peek.Blocks",
                  "item=%p block %d/%d (was %d) text=\"%s\"",
                  item, g_blockIdx, g_blockCount, prev, spoken);
    return true;
}

// Per-slot peek for the equip slot buttons (all 9, plus KOTOR 2's two
// second-weapon-set slots for free). The slot is identified by index in
// the engine's embedded slot-button array (EquipSlotIndexFromButton —
// pointer arithmetic, immune to variant .gui id renumbering; see
// docs/gui-id-audit.md), and the item handle is the parallel array entry
// at kEquipPanelSlotItemIdsOffset + 4*index — the panel-cached client
// handle the engine rewrites on every party-cycle, so the description
// always matches the displayed character.
//
// True on a non-empty description spoken; false on empty slot / unresolved
// item. Caller still consumes the key (predictable-behaviour rule).
bool HandleEquipSlotTooltip(void* panel, int slotIdx, bool down) {
    if (!panel || slotIdx < 0) return false;
    uint32_t handle = 0;
    if (!acc::engine::TryReadU32(
            panel, kEquipPanelSlotItemIdsOffset + 4u * static_cast<size_t>(slotIdx),
            &handle)) {
        acclog::Write("Peek.EquipSlot",
                      "panel=%p slot_index=%d SEH reading slot id",
                      panel, slotIdx);
        return false;
    }
    // 0x7f000000 = kInvalidObjectId (engine's "slot empty" sentinel).
    if (handle == 0 || handle == 0xffffffff || handle == 0x7f000000) {
        acclog::Write("Peek.EquipSlot",
                      "panel=%p slot_index=%d slot empty (handle=0x%x); silent",
                      panel, slotIdx, handle);
        return false;
    }

    // Panel-cached slot ids are client-side (high bit 0x80000000 set).
    void* item = acc::engine::ResolveItemFromClientHandle(handle);
    if (!item) {
        acclog::Write("Peek.EquipSlot",
                      "panel=%p slot_index=%d handle=0x%x; item not resolvable",
                      panel, slotIdx, handle);
        return false;
    }

    acclog::Write("Peek.EquipSlot",
                  "panel=%p slot_index=%d handle=0x%x item=%p -> block nav",
                  panel, slotIdx, handle, item);
    return SpeakItemBlocks(item, down);
}

// Shift+arrow on a workbench upgrade slot button (upgrade.gui IDs 12..18):
// speak the installed mod's full property description ("what the slot does
// for your item"), or announce the slot is empty. The installed mod lives at
// CSWGuiUpgrade.field35_0x2f74[slot_btn.custom_value] (engine-constructed from
// the mod template — see OnPanelAdded). Mirrors HandleEquipSlotTooltip; caller
// consumes the key regardless of return so behaviour is predictable.
bool HandleWorkbenchSlotTooltip(void* panel, void* control, bool down) {
    void* installed =
        acc::engine::GetWorkbenchSlotInstalledItem(panel, control);
    if (!installed) {
        acc::speech::Speak(
            acc::strings::Get(acc::strings::Id::WorkbenchSlotPeekEmpty),
            /*interrupt=*/true);
        acclog::Write("Peek.Workbench",
                      "panel=%p control=%p slot empty", panel, control);
        return true;
    }

    if (SpeakItemBlocks(installed, down)) {
        acclog::Write("Peek.Workbench",
                      "panel=%p control=%p item=%p -> block nav",
                      panel, control, installed);
        return true;
    }

    char text[4096];
    // Occupied but no property text — fall back to the mod's name so the peek
    // still says what's installed rather than going silent.
    if (acc::engine::ExtractTextOrStrRef(installed, kItemLocNameOffset,
                                         kItemLocNameOffset + 4, text,
                                         sizeof(text)) && text[0]) {
        acc::speech::Speak(text, /*interrupt=*/true);
        acclog::Write("Peek.Workbench",
                      "panel=%p control=%p item=%p name-fallback=\"%s\"",
                      panel, control, installed, text);
    } else {
        acclog::Write("Peek.Workbench",
                      "panel=%p control=%p item=%p no description/name",
                      panel, control, installed);
    }
    return true;
}

// Membership rule: this table is ONLY for panels whose rows are navigated by
// DriveListBoxSelection, where the listbox's selection_index IS the user's
// cursor. HandleItemTooltip below reads that field and nothing else.
//
// A chain-navigated panel (such as InGameQuestItems) must never be listed
// here: its cursor is g_chainIndex and selection_index stays wherever the
// engine last left it, so every peek would describe that one row. Those go
// through the chain-focus branch in HandleShiftArrow.
constexpr ItemTooltipPanelInfo kItemTooltipPanels[] = {
    { acc::engine::PanelKind::Container,        ContainerFindLb,        0 },
    { acc::engine::PanelKind::InGameEquip,      InGameEquipFindLb,      1 },
    { acc::engine::PanelKind::WorkbenchItems,   WorkbenchItemsFindLb,   0 },
    { acc::engine::PanelKind::WorkbenchUpgrade, WorkbenchUpgradeFindLb, 0 },
};

const ItemTooltipPanelInfo* LookupItemTooltipPanel(acc::engine::PanelKind k) {
    for (const auto& p : kItemTooltipPanels) {
        if (p.kind == k) return &p;
    }
    return nullptr;
}

// Caller consumes the key regardless of return (predictable behaviour).
bool HandleItemTooltip(acc::engine::PanelKind kind,
                       const ItemTooltipPanelInfo& info,
                       void* activePanel, bool down) {
    void* lb = info.findLb ? info.findLb(activePanel) : nullptr;
    if (!lb) {
        acclog::Write("Peek.Item",
                      "panel=%s no listbox; silent",
                      acc::engine::PanelKindName(kind));
        return false;
    }
    const int rowCount = acc::engine::ReadListBoxRowCount(lb);
    const int selIdx = acc::engine::ReadListBoxSelection(lb);

    if (rowCount <= 0 || selIdx < info.minSel || selIdx >= rowCount) {
        acclog::Write("Peek.Item",
                      "panel=%s lb=%p no focused item (sel=%d rows=%d "
                      "minSel=%d); silent",
                      acc::engine::PanelKindName(kind), lb,
                      (int)selIdx, rowCount, info.minSel);
        return false;
    }

    void* row = acc::engine::ReadListBoxRow(lb, selIdx);
    uint32_t clientHandle = 0;
    if (row && !acc::engine::TryReadU32(row, kStoreItemEntryObjIdOffset, &clientHandle)) {
        acclog::Write("Peek.Item", "panel=%s row=%p item handle unreadable",
                      acc::engine::PanelKindName(kind), row);
        return false;
    }

    // Workbench saber picker: prefer the engine's own hover description, which
    // prepends the keyed bonus line that GetPropertyDescription drops for
    // crystals. Falls through to GetPropertyDescription for non-saber items
    // (where that path is richer) and on empty result.
    if (kind == acc::engine::PanelKind::WorkbenchUpgrade) {
        char wb[8192];
        if (TryReadWorkbenchSaberDescription(activePanel, row, wb, sizeof(wb))) {
            // Lead with the "installed" marker when peeking the crystal that's
            // already in (or the colour already set on) the slot.
            auto pick = acc::engine::GetWorkbenchPickerInfo(activePanel);
            bool installed = pick.valid && pick.installedRow >= 0 &&
                             (int)selIdx == pick.installedRow;
            if (installed) {
                char marked[8192];
                snprintf(marked, sizeof(marked), "%s\n%s",
                         acc::strings::Get(
                             acc::strings::Id::WorkbenchPickerInstalled),
                         wb);
                acc::speech::Speak(marked, /*interrupt=*/true);
            } else {
                acc::speech::Speak(wb, /*interrupt=*/true);
            }
            acclog::Write("Peek.Workbench",
                          "panel=%s saber picker sel=%d/%d row=%p installed=%d "
                          "text=\"%s\"",
                          acc::engine::PanelKindName(kind), (int)selIdx,
                          rowCount, row, installed ? 1 : 0, wb);
            return true;
        }
    }

    void* item = acc::engine::ResolveItemFromClientHandle(clientHandle);
    if (!item) {
        acclog::Write("Peek.Item",
                      "panel=%s row=%p handle=0x%x; item not resolvable",
                      acc::engine::PanelKindName(kind), row, clientHandle);
        return false;
    }

    acclog::Write("Peek.Item",
                  "panel=%s row sel=%d/%d item=%p handle=0x%x -> block nav",
                  acc::engine::PanelKindName(kind), (int)selIdx, rowCount,
                  item, clientHandle);
    return SpeakItemBlocks(item, down);
}

bool SpeakItemRowDescription(void* row) {
    if (!row) return false;
    uint32_t clientHandle = 0;
    if (!acc::engine::TryReadU32(row, kStoreItemEntryObjIdOffset, &clientHandle)) {
        acclog::Write("Peek.Item", "row=%p item handle unreadable", row);
        return false;
    }
    void* item = acc::engine::ResolveItemFromClientHandle(clientHandle);
    if (!item) {
        acclog::Write("Peek.Item",
                      "row=%p handle=0x%x; item not resolvable",
                      row, clientHandle);
        return false;
    }
    char text[4096];
    if (!acc::engine::ReadItemPropertyDescription(item, text, sizeof(text))) {
        acclog::Write("Peek.Item", "row=%p item=%p empty description",
                      row, item);
        return false;
    }
    acc::speech::Speak(text, /*interrupt=*/true);
    acclog::Write("Peek.Item", "row=%p item=%p text=\"%s\"", row, item, text);
    return true;
}

}  // namespace acc::peek
