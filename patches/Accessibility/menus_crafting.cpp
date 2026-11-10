// The K2 workbench screens as a whole (menus_crafting.h has the design):
// the per-tick watcher with the auto-open redirect, the title announce on
// a foreground edge, the Q/E view toggle, and the chain filters.

#include <cstdio>
#include <cstring>

#include "menus_crafting_internal.h"

#include "engine_game.h"     // IsKotor2
#include "engine_manager.h"  // GetGuiManager, GetForegroundPanel
#include "engine_panels.h"   // IdentifyPanel, PanelKind
#include "engine_reads.h"    // ReadListBoxRowCount
#include "hotkeys.h"
#include "log.h"
#include "menus_chain.h"     // RebindChainPreserveIndex
#include "menus_extract.h"   // FromControl: the title label
#include "menus_internal.h"  // the ctor-bound control resolvers, FindControlById
#include "menus_pending.h"   // QueueActivate
#include "speech.h"
#include "strings.h"

using acc::engine::IdentifyPanel;
using acc::engine::PanelKind;

namespace acc::menus::crafting {

namespace {

// The controls this module drives resolve through the panels' ctor-bound
// members (menus_internal.cpp). These two titles, on panels whose members
// were not mined, are still looked up by .gui id; a wrong id costs a title
// line, never an input.
constexpr int kItemsTitleId   = 3;   // upgradeitems_p LBL_TITLE
constexpr int kUpgradeTitleId = 12;  // upgrade_p LBL_TITLE (the item name)

// upgrade_p.gui bakes this literal into LBL_TITLE; the engine replaces it
// with the item's name when it primes the panel, so it means "not yet".
constexpr const char* kUpgradeTitlePlaceholder = "Item Name";

// One redirect per workbench visit: reset when the whole family leaves the
// foreground, so a reopen within the visit (the user picks another item)
// is never bounced.
bool g_redirectedThisOpen = false;

void* TitleLabelFor(void* panel, PanelKind k) {
    switch (k) {
        case PanelKind::WorkbenchSelect:
            return acc::menus::detail::UpgradeSelPanelTitleLabel(panel);
        case PanelKind::WorkbenchItems:
            return acc::menus::detail::FindControlById(panel, kItemsTitleId);
        case PanelKind::WorkbenchUpgrade:
            return acc::menus::detail::FindControlById(panel, kUpgradeTitleId);
        case PanelKind::WorkbenchCreateItem:
        case PanelKind::WorkbenchCreateMedical:
            return acc::menus::detail::CraftPanelTitleLabel(panel);
        default:
            return nullptr;
    }
}

// The family panel's title when it arrives in the foreground (see
// OwnsPanelTitle). The upgrade screen says "Upgrade: <item>" and, like an
// empty label, retries next tick until the engine has written the name.
void AnnounceFamilyTitleOnFgEdge(void* fg, PanelKind pk, bool edge, bool suppress) {
    static bool s_retryTitle = false;
    if (!fg || !IsWorkbenchFamilyKind(pk) || suppress) {
        s_retryTitle = false;  // a bounced panel speaks nothing; the list will
        return;
    }
    if (!edge && !s_retryTitle) return;
    void* label = TitleLabelFor(fg, pk);
    char text[256];
    if (!label || !acc::menus::extract::FromControl(label, text, sizeof(text), fg) ||
        text[0] == '\0' ||
        (pk == PanelKind::WorkbenchUpgrade && strcmp(text, kUpgradeTitlePlaceholder) == 0)) {
        s_retryTitle = true;
        return;
    }
    char msg[320];
    if (pk == PanelKind::WorkbenchUpgrade) {
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtWorkbenchUpgradeTitle), text);
    } else {
        strncpy_s(msg, text, _TRUNCATE);
    }
    acc::speech::Speak(msg, /*interrupt=*/false);
    acclog::Write("Crafting", "fg-edge title panel=%p kind=%d -> \"%s\"",
                  fg, static_cast<int>(pk), msg);
    s_retryTitle = false;
}

// The K2 workbench dialog reply pushes CSWGuiUpgrade (the first
// upgradeable item's slots) on top of the item list, so keyboard users land
// inside "Upgrade: <first item>" instead of the list. The push cannot be
// stopped, so it is bounced through its BTN_BACK, unless the user opened it
// from the list (g_userOpenedUpgradeTtl). Returns true when it bounced, so
// the title stays silent.
bool RedirectAutoOpenedUpgrade(void* fg, PanelKind pk, bool edge) {
    if (g_userOpenedUpgradeTtl > 0) --g_userOpenedUpgradeTtl;
    if (!IsWorkbenchFamilyKind(pk)) g_redirectedThisOpen = false;
    if (!edge || pk != PanelKind::WorkbenchUpgrade) return false;
    if (g_userOpenedUpgradeTtl > 0) {
        g_userOpenedUpgradeTtl = 0;  // opened from the list: consume the marker
        g_redirectedThisOpen = true;
        return false;
    }
    if (g_redirectedThisOpen) return false;
    void* back = acc::menus::detail::UpgradePanelBackButton(fg);
    if (!back || acc::menus::pending::IsPending()) return false;
    acc::menus::pending::QueueActivate(back);
    g_redirectedThisOpen = true;
    acclog::Write("Crafting", "engine auto-opened Upgrade for first item; "
                  "redirecting to item list (BTN_BACK=%p panel=%p)", back, fg);
    return true;
}

// A create/breakdown flip is spoken and both it and a changed row count
// (category filter, an item made or broken down) rebind the chain. A fresh
// open latches silently: first sight covers it.
void WatchList(void* fg, PanelKind pk) {
    static void* s_lastPanel   = nullptr;
    static int   s_lastShopVis = -1;  // 1 = create view, 0 = breakdown view
    static int   s_lastCount   = -1;
    const bool isCraft = IsCraftingKind(pk);
    const bool isSel = pk == PanelKind::WorkbenchSelect && acc::game::IsKotor2();
    if (!isCraft && !isSel) {
        s_lastPanel = nullptr;
        return;
    }
    void* lb = nullptr;
    int shopVis = -1;
    if (isCraft) {
        void* shop = acc::menus::detail::CraftPanelShopListBox(fg);
        shopVis = (shop && IsListBoxVisible(shop)) ? 1 : 0;
        lb = FindActiveCraftList(fg);
    } else {
        lb = acc::menus::detail::UpgradeSelPanelListBox(fg);
    }
    const int count = acc::engine::ReadListBoxRowCount(lb);
    if (fg != s_lastPanel) {
        s_lastPanel = fg;
    } else if (isCraft && shopVis != -1 && s_lastShopVis != -1 && shopVis != s_lastShopVis) {
        acc::speech::Speak(acc::strings::Get(shopVis ? acc::strings::Id::WorkbenchModeCreate
                                                     : acc::strings::Id::WorkbenchModeBreakdown),
                           /*interrupt=*/false);
        acclog::Write("Crafting", "view flip panel=%p shopVis=%d -> rebind", fg, shopVis);
        acc::menus::chain::RebindChainPreserveIndex(fg);
    } else if (count != s_lastCount) {
        acclog::Write("Crafting", "list count %d -> %d panel=%p lb=%p -> rebind",
                      s_lastCount, count, fg, lb);
        acc::menus::chain::RebindChainPreserveIndex(fg);
    }
    s_lastShopVis = shopVis;
    s_lastCount   = count;
}

}  // namespace

int g_userOpenedUpgradeTtl = 0;

bool IsCraftingKind(PanelKind k) {
    return k == PanelKind::WorkbenchCreateItem || k == PanelKind::WorkbenchCreateMedical;
}

bool IsWorkbenchFamilyKind(PanelKind k) {
    return k == PanelKind::WorkbenchSelect || k == PanelKind::WorkbenchItems ||
           k == PanelKind::WorkbenchUpgrade || IsCraftingKind(k);
}

bool IsListBoxVisible(void* listBox) {
    return acc::engine::IsControlVisible(listBox);
}

// LB_SHOPITEMS in the create view, LB_INVITEMS in the breakdown view; null
// when neither is visible (still building) or off the crafting screens.
void* FindActiveCraftList(void* panel) {
    void* shop = acc::menus::detail::CraftPanelShopListBox(panel);
    if (shop && IsListBoxVisible(shop)) return shop;
    void* inv = acc::menus::detail::CraftPanelInvListBox(panel);
    if (inv && IsListBoxVisible(inv)) return inv;
    return nullptr;
}

// Null off the two crafting screens (and on K1, where the member offsets
// poison).
void* ViewToggleButton(void* panel) {
    return acc::menus::detail::CraftPanelExamineButton(panel);
}

void* CommitButton(void* panel) {
    return acc::menus::detail::CraftPanelAcceptButton(panel);
}

// Like store::ToggleModeFromHotkey: the engine's own button, fired from a
// key; Tick's view-flip edge speaks the result.
bool ToggleViewFromHotkey() {
    void* mgr = acc::engine::GetGuiManager();
    void* fg = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    void* btn = ViewToggleButton(fg);
    if (!btn) return false;
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Crafting", "Q/E (view toggle) -- op already pending; ignoring");
        return false;
    }
    acc::menus::pending::QueueActivate(btn);
    acclog::Write("Crafting", "Q/E (view toggle) -> FireActivate BTN_Examine panel=%p target=%p",
                  fg, btn);
    return true;
}

bool IsHiddenCraftingListBox(void* panel, void* listBox) {
    if (!panel || !listBox) return false;
    if (listBox != acc::menus::detail::CraftPanelShopListBox(panel) &&
        listBox != acc::menus::detail::CraftPanelInvListBox(panel)) {
        return false;
    }
    return !IsListBoxVisible(listBox);
}

bool OwnsPanelTitle(void* panel) {
    return acc::game::IsKotor2() && IsWorkbenchFamilyKind(IdentifyPanel(panel));
}

void Tick() {
    static void* s_lastFg = nullptr;  // one edge for the redirect and the title
    void* mgr = acc::engine::GetGuiManager();
    void* fg = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    const PanelKind pk = fg ? IdentifyPanel(fg) : PanelKind::Unknown;

    // Polled: the engine eats these scancodes before the menu dispatcher.
    // ToggleViewFromHotkey checks the foreground itself, which keeps it out
    // of the container's and store's way on the same keys.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::CraftViewToggle)) ToggleViewFromHotkey();

    const bool edge = fg != s_lastFg;
    s_lastFg = fg;
    if (acc::game::IsKotor2()) {
        const bool bounced = RedirectAutoOpenedUpgrade(fg, pk, edge);
        AnnounceFamilyTitleOnFgEdge(fg, pk, edge, bounced);
    }
    WatchList(fg, pk);
}

}  // namespace acc::menus::crafting
