// The store (CSWGuiStore; menus_store.h has the design): buy/sell mode from
// the visible item list, the price and stock suffix on a chain step, the
// trade dispatch with its outcome watcher, and the G and Esc keys.

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "menus_store.h"

#include "engine_manager.h"  // GetGuiManager, GetForegroundPanel
#include "engine_offsets.h"
#include "engine_reads.h"    // TryRead*, ReadListBoxRowCount, ResolveItemFromClientHandle
#include "hotkeys.h"
#include "log.h"
#include "menus_chain.h"     // RebindChain on a mode flip
#include "menus_pending.h"   // QueueActivate, RaiseIsActiveIfZero
#include "speech.h"
#include "strings.h"

namespace acc::menus::store {

namespace {

using acc::engine::TryReadPtr;
using acc::engine::TryReadU16;
using acc::engine::TryReadU32;

enum class Mode { Unknown, Buy, Sell };

using PFN_GetItemValue = uint32_t(__thiscall*)(void* self, void* item);
using PFN_StoreOnControlButton = void(__thiscall*)(void* self, void* row);

const char* ModeName(Mode m) { return m == Mode::Buy ? "buy" : "sell"; }

size_t ListOffset(Mode m) {
    return m == Mode::Buy ? kStoreShopItemsListBoxOffset : kStoreInvItemsListBoxOffset;
}

// The engine sets the visible bit on whichever item list shows
// (ShowBuyGUI / ShowSellGUI); a failed read counts as hidden.
bool ListVisible(void* panel, size_t listOffset) {
    uint32_t bits = 0;
    return TryReadU32(static_cast<unsigned char*>(panel) + listOffset, kControlBitFlagsOffset,
                      &bits) &&
           (bits & kStoreListBoxVisibleBit) != 0;
}

// Unknown only in the moment before InitializeStoreType runs.
Mode ResolveMode(void* panel) {
    if (!IsStorePanel(panel)) return Mode::Unknown;
    if (ListVisible(panel, kStoreShopItemsListBoxOffset)) return Mode::Buy;
    if (ListVisible(panel, kStoreInvItemsListBoxOffset)) return Mode::Sell;
    return Mode::Unknown;
}

int ListSize(void* panel, Mode m) {
    return acc::engine::ReadListBoxRowCount(static_cast<unsigned char*>(panel) + ListOffset(m));
}

uint32_t ReadU32Or0(void* base, size_t offset) {
    uint32_t v = 0;
    TryReadU32(base, offset, &v);
    return v;
}

// The item behind a store row (CSWGuiStoreItemEntry).
void* RowItem(void* row) {
    return acc::engine::ResolveItemFromClientHandle(ReadU32Or0(row, kStoreItemEntryObjIdOffset));
}

// GetItemBuyValue / GetItemSellValue on the live panel; 0 on a fault.
uint32_t ItemPrice(void* panel, Mode m, void* item) {
    if (!panel || !item) return 0;
    const uintptr_t fn = m == Mode::Buy ? kAddrCSWGuiStoreGetItemBuyValue
                                        : kAddrCSWGuiStoreGetItemSellValue;
    __try {
        return reinterpret_cast<PFN_GetItemValue>(fn)(panel, item);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// CSWSItem stack_size, a ushort (the engine reads it as an int; the upper
// half is padding). outFinite is false for unlimited stock, the item flag
// OnControlEntered branches on.
int ReadItemStock(void* item, bool& outFinite) {
    outFinite = true;
    uint32_t bitFlags = 0;
    uint16_t stack = 0;
    if (!TryReadU32(item, kSwsItemBitFlagsOffset, &bitFlags) ||
        !TryReadU16(item, kSwsItemStackSizeOffset, &stack)) {
        return 0;
    }
    if (bitFlags & kSwsItemInfiniteStockBit) {
        outFinite = false;
        return 0;
    }
    return stack;
}

void* ForegroundStore() {
    void* mgr = acc::engine::GetGuiManager();
    void* fg = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    return IsStorePanel(fg) ? fg : nullptr;
}

// Cleared when the store leaves the foreground, so a reopened merchant
// starts fresh.
void* g_lastSeenStorePanel = nullptr;
Mode  g_lastSeenMode       = Mode::Unknown;
// The visible list's size last tick. A trade repopulates the list in place
// (the row pointers stay, their items change), so a size change rebinds the
// chain.
int   g_lastSeenActiveListBoxSize = -1;

// The outcome watcher, armed by DispatchTradeAction. The gold the panel
// caches is the signal: BuyItem lowers it and SellItem raises it before the
// next tick. The list size is not, as a row only drops when its stock or
// stack crosses zero (a multi-stock buy, or selling one of a stack, keeps
// it); it stays a fallback for a price-0 trade. A mode flip drops the watch.
struct TradeWatch {
    bool     armed          = false;
    Mode     mode           = Mode::Unknown;
    int      sizeAtArm      = -1;
    int      ticksRemaining = 0;
    uint32_t price          = 0;  // for "Sold for 16 credits"
    uint32_t goldAtArm      = 0;
};
TradeWatch g_watch;

// The engine commits synchronously, so a few quiet ticks mean a refusal.
constexpr int kTradeWatchTicks = 4;

void ResetTracking() {
    g_lastSeenStorePanel = nullptr;
    g_lastSeenMode = Mode::Unknown;
    g_lastSeenActiveListBoxSize = -1;
    g_watch.armed = false;
}

// The chain is panel-keyed and the panel stays, so without a rebind the user
// would walk the hidden list. The cursor goes to the first row: the two
// lists hold different items.
void OnModeChange(void* fg, Mode current) {
    g_lastSeenMode = current;
    acc::speech::Speak(acc::strings::Get(current == Mode::Buy ? acc::strings::Id::StoreModeBuy
                                                              : acc::strings::Id::StoreModeSell),
                       /*interrupt=*/false);
    acclog::Write("Menus.Store", "mode change panel=%p -> %s", fg, ModeName(current));
    acc::menus::chain::RebindChain(fg);
    acc::menus::chain::g_chainIndex = 0;
    g_lastSeenActiveListBoxSize = ListSize(fg, current);
    g_watch.armed = false;
}

void ResolveTradeWatch(void* fg, Mode current, bool sizeChanged, int currentSize) {
    if (!g_watch.armed || g_watch.mode != current) return;
    const uint32_t gold = ReadU32Or0(fg, kStorePlayerGoldOffset);
    const bool goldMoved = current == Mode::Buy ? gold < g_watch.goldAtArm
                                                : gold > g_watch.goldAtArm;
    if (goldMoved || sizeChanged) {
        // An unchanged size means the row's number changed in place: rebind
        // so the chain re-reads it.
        if (!sizeChanged) acc::menus::chain::RebindChainPreserveIndex(fg);
        char msg[96];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(current == Mode::Buy ? acc::strings::Id::FmtStoreBoughtFor
                                                        : acc::strings::Id::FmtStoreSoldFor),
                 static_cast<int>(g_watch.price));
        acc::speech::Speak(msg, /*interrupt=*/false);
        acclog::Write("Menus.Store", "trade success mode=%s via %s (gold %u -> %u, "
                      "size delta=%d, price=%u)", ModeName(current), goldMoved ? "gold" : "size",
                      g_watch.goldAtArm, gold, sizeChanged ? currentSize - g_watch.sizeAtArm : 0,
                      g_watch.price);
        g_watch.armed = false;
        return;
    }
    if (g_watch.ticksRemaining > 0) {
        --g_watch.ticksRemaining;
        return;
    }
    acc::speech::Speak(acc::strings::Get(current == Mode::Buy ? acc::strings::Id::StoreCannotBuy
                                                              : acc::strings::Id::StoreCannotSell),
                       /*interrupt=*/false);
    acclog::Write("Menus.Store", "trade refused mode=%s (no gold/size change after watch; gold=%u)",
                  ModeName(current), gold);
    g_watch.armed = false;
}

}  // namespace

bool IsStorePanel(void* panel) {
    return acc::engine::HasVtable(panel, kVtableCSWGuiStore);
}

bool IsStoreItemRow(void* control) {
    return acc::engine::HasVtable(control, kVtableCSWGuiStoreItemEntry);
}

bool IsHiddenStoreListBox(void* panel, void* listBox) {
    if (!IsStorePanel(panel) || !listBox) return false;
    const ptrdiff_t off = static_cast<unsigned char*>(listBox) - static_cast<unsigned char*>(panel);
    if (off != static_cast<ptrdiff_t>(kStoreShopItemsListBoxOffset) &&
        off != static_cast<ptrdiff_t>(kStoreInvItemsListBoxOffset)) {
        return false;  // the description listbox and the like
    }
    return !ListVisible(panel, static_cast<size_t>(off));
}

void AnnounceChainStepSuffix(void* panel, void* control) {
    if (!IsStorePanel(panel) || !IsStoreItemRow(control)) return;
    const Mode mode = ResolveMode(panel);
    if (mode == Mode::Unknown) return;
    void* item = RowItem(control);
    if (!item) {
        acclog::Write("Menus.Store", "chain-step suffix focus=%p handle=0x%x: item resolve failed",
                      control, ReadU32Or0(control, kStoreItemEntryObjIdOffset));
        return;
    }
    const uint32_t price = ItemPrice(panel, mode, item);
    bool finite = true;
    const int stock = ReadItemStock(item, finite);
    using acc::strings::Id;
    char msg[160];
    if (mode == Mode::Buy && !finite) {
        snprintf(msg, sizeof(msg), acc::strings::Get(Id::FmtStorePriceBuyUnlimited),
                 static_cast<int>(price));
    } else {
        // Selling, the stock is how many the player owns.
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(mode == Mode::Buy ? Id::FmtStorePriceBuyFinite
                                                     : Id::FmtStorePriceSell),
                 static_cast<int>(price), stock);
    }
    acc::speech::Speak(msg, /*interrupt=*/false);
    // A charged item, sold or bought, also says its charges.
    const int charges = acc::engine::ReadItemCharges(item);
    if (charges >= 0) {
        char chargeMsg[64];
        snprintf(chargeMsg, sizeof(chargeMsg), acc::strings::Get(Id::FmtItemChargeSuffix), charges);
        acc::speech::Speak(chargeMsg, /*interrupt=*/false);
    }
    acclog::Write("Menus.Store", "chain-step suffix focus=%p mode=%s price=%u stock=%d finite=%d "
                  "charges=%d", control, ModeName(mode), price, stock, finite ? 1 : 0, charges);
}

void TickMonitorMode() {
    // G is polled here, beside the store state: the engine eats unbound
    // scancodes before the menu dispatcher.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::StoreModeToggle)) ToggleModeFromHotkey();

    void* fg = ForegroundStore();
    if (!fg) {
        if (g_lastSeenStorePanel) ResetTracking();
        return;
    }
    const Mode current = ResolveMode(fg);
    if (current == Mode::Unknown) return;
    if (fg != g_lastSeenStorePanel) {
        // Just opened: the panel title already covers the mode.
        g_lastSeenStorePanel = fg;
        g_lastSeenMode = current;
        g_lastSeenActiveListBoxSize = ListSize(fg, current);
        g_watch.armed = false;
        return;
    }
    if (current != g_lastSeenMode) {
        OnModeChange(fg, current);
        return;
    }
    const int currentSize = ListSize(fg, current);
    const bool sizeChanged = currentSize >= 0 && currentSize != g_lastSeenActiveListBoxSize;
    if (sizeChanged) {
        acclog::Write("Menus.Store",
                      "active list size %d -> %d (mode=%s); chain rebind preserving idx",
                      g_lastSeenActiveListBoxSize, currentSize, ModeName(current));
        g_lastSeenActiveListBoxSize = currentSize;
        acc::menus::chain::RebindChainPreserveIndex(fg);
    }
    ResolveTradeWatch(fg, current, sizeChanged, currentSize);
}

void DispatchTradeAction(void* panel, void* row) {
    if (!IsStorePanel(panel) || !IsStoreItemRow(row)) {
        acclog::Write("Menus.Store", "DispatchTradeAction: not store/row (panel=%p row=%p) skip",
                      panel, row);
        return;
    }
    const Mode mode = ResolveMode(panel);
    if (mode == Mode::Unknown) {
        acclog::Write("Menus.Store", "DispatchTradeAction: unknown mode (panel=%p) skip", panel);
        return;
    }
    void* item = RowItem(row);
    const uint32_t price = ItemPrice(panel, mode, item);

    // Not enough credits: said and skipped. The engine's own answer is a
    // visual-only examine box outside the chain, a stranded modal.
    if (mode == Mode::Buy && item) {
        const uint32_t gold = ReadU32Or0(panel, kStorePlayerGoldOffset);
        if (gold < price) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     acc::strings::Get(acc::strings::Id::FmtStoreNotEnoughCredits),
                     static_cast<int>(price), static_cast<int>(gold));
            acc::speech::Speak(msg, /*interrupt=*/false);
            acclog::Write("Menus.Store",
                          "DispatchTradeAction buy refused (gold=%u price=%u) panel=%p",
                          gold, price, panel);
            g_watch.armed = false;
            return;
        }
    }

    // Armed before the call, so a synchronous commit resolves next tick.
    g_watch = TradeWatch{true, mode, ListSize(panel, mode), kTradeWatchTicks, price,
                         ReadU32Or0(panel, kStorePlayerGoldOffset)};

    // OnControl{Store,Inv}AButton do nothing at all unless the row's
    // is_active is set, which a mouse click sets before the dispatch; that
    // early-out was a false "Cannot be bought". It is the engine's "hot
    // control" state, not a buyable flag, so raising it only mimics the
    // mouse.
    uint32_t prevRowActive = 0;
    __try {
        prevRowActive = acc::menus::pending::RaiseIsActiveIfZero(row);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    const uintptr_t fnAddr = mode == Mode::Buy ? kAddrCSWGuiStoreOnControlStoreAButton
                                               : kAddrCSWGuiStoreOnControlInvAButton;
    acclog::Write("Menus.Store", "DispatchTradeAction panel=%p row=%p mode=%s -> %s "
                  "(is_active=%u%s, item_resolved=%d, watch size=%d, ticks=%d, price=%u)",
                  panel, row, ModeName(mode),
                  mode == Mode::Buy ? "OnControlStoreAButton" : "OnControlInvAButton",
                  prevRowActive, prevRowActive == 0 ? "->1" : " (preserved)",
                  item != nullptr ? 1 : 0, g_watch.sizeAtArm, g_watch.ticksRemaining, price);
    __try {
        reinterpret_cast<PFN_StoreOnControlButton>(fnAddr)(panel, row);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Menus.Store", "DispatchTradeAction SEH fault panel=%p row=%p", panel, row);
        g_watch.armed = false;
    }
}

bool ToggleModeFromHotkey() {
    void* fg = ForegroundStore();
    if (!fg) return false;
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Menus.Store", "G (mode toggle) -- op already pending; ignoring");
        return false;
    }
    void* toggleBtn = static_cast<unsigned char*>(fg) + kStoreToggleButtonOffset;
    acc::menus::pending::QueueActivate(toggleBtn);
    acclog::Write("Menus.Store", "G (mode toggle) -> FireActivate toggle_button panel=%p target=%p",
                  fg, toggleBtn);
    return true;
}

bool CloseFromEsc() {
    void* fg = ForegroundStore();
    if (!fg) return false;
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Menus.Store", "Esc (close) -- op already pending; ignoring");
        return false;
    }
    void* cancelBtn = static_cast<unsigned char*>(fg) + kStoreCancelButtonOffset;
    acc::menus::pending::QueueActivate(cancelBtn);
    acclog::Write("Menus.Store", "Esc (close) -> FireActivate cancel_button panel=%p target=%p",
                  fg, cancelBtn);
    return true;
}

}  // namespace acc::menus::store
