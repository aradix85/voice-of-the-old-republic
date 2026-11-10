// KOTOR 2 workbench / crafting item-list screens.
// See menus_crafting.h for the design rationale + decompile evidence.

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "menus_crafting_internal.h"

#include "engine_game.h"      // IsKotor2 — WorkbenchSelect row commit is K2-only
#include "engine_manager.h"   // GetForegroundPanel, kAddrGuiManagerPtr
#include "engine_offsets.h"
#include "engine_panels.h"    // IdentifyPanel, PanelKind
#include "engine_reads.h"     // ResolveItemFromClientHandle — row -> CSWSItem*
#include "log.h"
#include "menus_chain.h"      // RebindChainPreserveIndex on list changes
#include "menus_pending.h"    // QueueActivate — auto-open redirect (BTN_BACK)
#include "menus_extract.h"    // FromControl — title label read
#include "hotkeys.h"          // Pressed() for Q/E (CraftViewToggle)
#include "menus_internal.h"   // detail::FindControlById
#include "strings.h"
#include "speech.h"

using acc::engine::IdentifyPanel;
using acc::engine::ReadListBoxRowCount;
using acc::engine::PanelKind;
using acc::menus::detail::FindControlById;

namespace acc::menus::crafting {

namespace {

// One redirect per workbench-open. Reset when the whole family leaves the
// foreground, so the next visit redirects again but a within-visit reopen
// (user Enters another item) never does.
bool g_redirectedThisOpen = false;

// SINCE the gui-id audit (2026-08-18, docs/gui-id-audit.md) the controls
// this module acts on resolve via the panels' own ctor-bound embedded
// members (UpgradeSelPanel* / CraftPanel* in menus_internal.cpp; offsets
// in engine_offsets_fields.h) — the historical .gui ids survive only as
// those resolvers' GuiIdMismatch tripwires. The two ids below are the
// remaining Tier-2 announce-only lookups on panels whose members were not
// mined (upgradeitems_p / upgrade_p titles); degradation there is a wrong
// or missing title line, never a mis-driven input.
constexpr int kItemsTitleId        = 3;   // upgradeitems_p LBL_TITLE
constexpr int kUpgradeTitleId      = 12;  // upgrade_p LBL_TITLE (item name)
// The crafting screen's own per-selection number labels, LBL_COST_VALUE (id 8) and
// LBL_STOCK_VALUE (id 11), are deliberately NOT read. Live evidence
// (patch-20260813-132834.log): they held "12" / "0" across every keyboard
// selection change and only moved when a craft committed — the engine
// refreshes them from the row's own mouse-enter event, which the keyboard
// path never fires. Quoting them per row would have announced a stale price.
// AnnounceChainStepSuffix calls the engine's cost function on the focused
// row's item instead, which cannot go stale. The pool label (id 6) IS
// reliable — it is written whenever the pool changes — and is surfaced as a
// virtual chain row by menus_credits.

// upgrade_p.gui bakes this literal (not a strref) into LBL_TITLE; the
// engine overwrites it with the item's name when it primes the panel.
// Locale-independent, so equality is a safe "not primed yet" test.
constexpr const char* kUpgradeTitlePlaceholder = "Item Name";

// The title label of a K2 workbench-family panel, or nullptr for
// non-family kinds. Select and the two crafting screens resolve through
// their ctor-bound members; Items and Upgrade keep the Tier-2 id lookup
// (their members were not mined — announce-only, degrades gracefully).
void* TitleLabelFor(void* panel, PanelKind k) {
    switch (k) {
        case PanelKind::WorkbenchSelect:
            return acc::menus::detail::UpgradeSelPanelTitleLabel(panel);
        case PanelKind::WorkbenchItems:
            return FindControlById(panel, kItemsTitleId);
        case PanelKind::WorkbenchUpgrade:
            return FindControlById(panel, kUpgradeTitleId);
        case PanelKind::WorkbenchCreateItem:
        case PanelKind::WorkbenchCreateMedical:
            return acc::menus::detail::CraftPanelTitleLabel(panel);
        default:
            return nullptr;
    }
}

// Speak the family panel's title when it ARRIVES in the foreground.
// Replaces pointer-keyed first-sight for these panels on K2: the stacked
// push spoke four construction-time titles in a row, and the pre-built
// panels are reused so re-entries stayed silent. The upgrade screen
// formats its title as "Upgrade: <item name>" and retries while the
// engine hasn't yet replaced the .gui placeholder.
void AnnounceFamilyTitleOnFgEdge(void* fg, PanelKind pk, bool edge,
                                 bool suppress) {
    static bool s_retryTitle = false;

    if (!fg || !IsWorkbenchFamilyKind(pk)) {
        s_retryTitle = false;
        return;
    }
    if (suppress) {
        // We're bouncing this panel back to the list; don't announce its
        // title, but clear any pending retry so the list's title speaks
        // cleanly on the next edge.
        s_retryTitle = false;
        return;
    }
    if (!edge && !s_retryTitle) return;

    void* label = TitleLabelFor(fg, pk);
    char text[256];
    if (!label ||
        !acc::menus::extract::FromControl(label, text, sizeof(text), fg) ||
        text[0] == '\0') {
        // Label empty or unreadable — try again next tick (same shape as
        // the placeholder retry; gives the engine a frame to populate).
        s_retryTitle = true;
        return;
    }

    if (pk == PanelKind::WorkbenchUpgrade) {
        if (strcmp(text, kUpgradeTitlePlaceholder) == 0) {
            s_retryTitle = true;
            return;
        }
        char msg[320];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtWorkbenchUpgradeTitle),
                 text);
        acc::speech::Speak(msg, /*interrupt=*/false);
        acclog::Write("Crafting", "fg-edge title panel=%p kind=%d -> \"%s\"",
                      fg, (int)pk, msg);
    } else {
        acc::speech::Speak(text, /*interrupt=*/false);
        acclog::Write("Crafting", "fg-edge title panel=%p kind=%d -> \"%s\"",
                      fg, (int)pk, text);
    }
    s_retryTitle = false;
}

}  // namespace

// The K2 workbench dialog reply pushes CSWGuiUpgrade (the slot detail for
// the FIRST upgradeable item) on TOP of the item list, so the user lands
// inside "Upgrade: <first item>" and has to Escape to reach the list —
// the engine's own flow, but the wrong entry point for keyboard users, who
// want the list first (reported 2026-08-12; verified engine-driven, no mod
// action precedes the push in patch-20260812-144927.log).
//
// We can't stop the push, so we bounce it: when CSWGuiUpgrade arrives in the
// foreground and the user did NOT open it themselves (no craft-row-commit
// from the list just fired), fire its BTN_BACK to drop to the list. This
// counter is set when the chain routes Enter on a list row to the upgrade
// flow; it survives a few ticks (the push lands a frame or two later) and is
// consumed by the arriving upgrade edge.
int  g_userOpenedUpgradeTtl = 0;

bool IsCraftingKind(PanelKind k) {
    return k == PanelKind::WorkbenchCreateItem ||
           k == PanelKind::WorkbenchCreateMedical;
}

bool IsWorkbenchFamilyKind(PanelKind k) {
    return k == PanelKind::WorkbenchSelect || k == PanelKind::WorkbenchItems ||
           k == PanelKind::WorkbenchUpgrade || IsCraftingKind(k);
}

bool IsListBoxVisible(void* listBox) {
    return acc::engine::IsControlVisible(listBox);
}

// The item list the user can currently act on: LB_SHOPITEMS in create
// view, LB_INVITEMS in breakdown view. Null when neither is visible
// (construction window) or the panel isn't a crafting screen.
void* FindActiveCraftList(void* panel) {
    void* shop = acc::menus::detail::CraftPanelShopListBox(panel);
    if (shop && IsListBoxVisible(shop)) return shop;
    void* inv = acc::menus::detail::CraftPanelInvListBox(panel);
    if (inv && IsListBoxVisible(inv)) return inv;
    return nullptr;
}

void* ViewToggleButton(void* panel) {
    // The resolver is kind-dispatched and returns nullptr off the two
    // crafting screens (and on K1, where the member offsets poison).
    return acc::menus::detail::CraftPanelExamineButton(panel);
}

void* CommitButton(void* panel) {
    return acc::menus::detail::CraftPanelAcceptButton(panel);
}

// Q / E — flip create <-> break down. Mirrors store::ToggleModeFromHotkey:
// the button is the engine's own, we just fire it from a key instead of
// leaving it in the chain. The resulting view flip is announced by Tick's
// existing shopVis edge, so nothing is spoken here.
bool ToggleViewFromHotkey() {
    void* mgr = nullptr;
    __try {
        mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!mgr) return false;
    void* fg = acc::engine::GetForegroundPanel(mgr);
    void* btn = ViewToggleButton(fg);
    if (!btn) return false;
    if (acc::menus::pending::IsPending()) {
        acclog::Write("Crafting",
                      "Q/E (view toggle) -- op already pending; ignoring");
        return false;
    }
    acc::menus::pending::QueueActivate(btn);
    acclog::Write("Crafting",
                  "Q/E (view toggle) -> FireActivate BTN_Examine panel=%p "
                  "target=%p", fg, btn);
    return true;
}

bool IsHiddenCraftingListBox(void* panel, void* listBox) {
    if (!panel || !listBox) return false;
    if (listBox != acc::menus::detail::CraftPanelShopListBox(panel) &&
        listBox != acc::menus::detail::CraftPanelInvListBox(panel)) {
        // Off-kind panels (resolvers null there), description listboxes
        // etc. — chain handles them normally.
        return false;
    }
    return !IsListBoxVisible(listBox);
}

bool OwnsPanelTitle(void* panel) {
    if (!acc::game::IsKotor2()) return false;
    return IsWorkbenchFamilyKind(IdentifyPanel(panel));
}

void Tick() {
    // Latched foreground state. Panel pointer distinguishes re-opens (a
    // fresh open must not announce the initial view as a "flip").
    static void* s_lastPanel   = nullptr;
    static int   s_lastShopVis = -1;   // 1 = create view, 0 = breakdown view
    static int   s_lastCount   = -1;   // active-list row count

    // Foreground-edge tracker shared by the title announcer and the
    // auto-open redirect (single s_lastFg so both see the same edge).
    static void* s_lastFg = nullptr;

    void* mgr = *reinterpret_cast<void**>(kAddrGuiManagerPtr);
    void* fg = mgr ? acc::engine::GetForegroundPanel(mgr) : nullptr;
    PanelKind pk = fg ? IdentifyPanel(fg) : PanelKind::Unknown;

    // Q/E view toggle. Polled here for the same reason the store polls its
    // own: the engine eats these scancodes before the menu dispatcher sees
    // them, so the keyboard has to be read directly. ToggleViewFromHotkey
    // re-checks the foreground itself, which is what keeps this key from
    // colliding with the container and store handlers on the same keys.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::CraftViewToggle)) {
        ToggleViewFromHotkey();
    }

    bool edge = fg != s_lastFg;
    s_lastFg = fg;

    if (acc::game::IsKotor2()) {
        if (g_userOpenedUpgradeTtl > 0) --g_userOpenedUpgradeTtl;
        // Reset the once-per-open redirect latch when the whole workbench
        // family has left the foreground.
        if (!IsWorkbenchFamilyKind(pk)) g_redirectedThisOpen = false;

        // Redirect the engine's auto-open of the first item's upgrade screen
        // back to the item list, unless the user opened it themselves.
        bool suppressTitle = false;
        if (edge && pk == PanelKind::WorkbenchUpgrade &&
            !g_redirectedThisOpen && g_userOpenedUpgradeTtl == 0) {
            void* back = acc::menus::detail::UpgradePanelBackButton(fg);
            if (back && !acc::menus::pending::IsPending()) {
                acc::menus::pending::QueueActivate(back);
                g_redirectedThisOpen = true;
                suppressTitle = true;
                acclog::Write("Crafting",
                              "engine auto-opened Upgrade for first item; "
                              "redirecting to item list (BTN_BACK=%p panel=%p)",
                              back, fg);
            }
        }
        if (edge && pk == PanelKind::WorkbenchUpgrade && g_userOpenedUpgradeTtl > 0) {
            // User drove this open from the list — consume the marker and
            // latch so nothing bounces it.
            g_userOpenedUpgradeTtl = 0;
            g_redirectedThisOpen = true;
        }
        AnnounceFamilyTitleOnFgEdge(fg, pk, edge, suppressTitle);
    }

    bool isCraft = IsCraftingKind(pk);
    bool isSel = pk == PanelKind::WorkbenchSelect && acc::game::IsKotor2();
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
    int count = ReadListBoxRowCount(lb);

    if (fg != s_lastPanel) {
        // Fresh open (or panel swap): latch silently. The first-sight
        // title + focused-control announce cover orientation.
        s_lastPanel   = fg;
        s_lastShopVis = shopVis;
        s_lastCount   = count;
        return;
    }

    bool rebind = false;
    if (isCraft && shopVis != -1 && s_lastShopVis != -1 &&
        shopVis != s_lastShopVis) {
        // BTN_Examine (or the engine itself) flipped create ↔ breakdown.
        acc::speech::Speak(acc::strings::Get(
                         shopVis ? acc::strings::Id::WorkbenchModeCreate
                                 : acc::strings::Id::WorkbenchModeBreakdown),
                     /*interrupt=*/false);
        acclog::Write("Crafting", "view flip panel=%p shopVis=%d -> rebind",
                      fg, shopVis);
        rebind = true;
    } else if (count != s_lastCount) {
        // Category filter switch or a create/breakdown changed the list.
        acclog::Write("Crafting",
                      "list count %d -> %d panel=%p lb=%p -> rebind",
                      s_lastCount, count, fg, lb);
        rebind = true;
    }
    if (rebind) {
        acc::menus::chain::RebindChainPreserveIndex(fg);
    }

    s_lastShopVis = shopVis;
    s_lastCount   = count;
}

}  // namespace acc::menus::crafting
