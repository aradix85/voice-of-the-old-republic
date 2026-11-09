// The save and load screens: the save list, Enter and Escape, and on
// KOTOR 2 the selection monitor, the save information and the character
// swap.

#include "menus_listbox_internal.h"

namespace acc::menus::listbox {

namespace {

// Set by the sync, consumed by the announce one tick later.
void* s_pendingInfoPanel = nullptr;

// ============================================================================
// SaveLoad — Spiel laden / Spiel speichern dialog.
// ============================================================================

// K1 only, deliberately. The spec consumes Up/Down outright to drive the
// listbox, which is right on K1 where the only two actions the screen needs —
// Load and Cancel — hang off Enter and Esc. KOTOR 2's version of the screen
// has five buttons (Cancel, Load, Delete, Switch Characters, Cloud Saves),
// and walking them is how the player reaches the last three; enabling the
// spec there swallowed the arrow keys and lost them (reported 2026-08-13,
// immediately after the id fix made the spec match K2 for the first time).
//
// K2 keeps ordinary chain navigation over rows AND buttons — the arrangement
// it always had — and gets correctness from SyncK2SaveLoadSelection below,
// which drives the engine's selection index to whatever row the chain is on.
// That was the actual defect: chain focus moved and spoke the row, but
// nothing told the engine, so Load always loaded the default (newest) save.
bool SaveLoadMatches(void* p) {
    return !acc::game::IsKotor2() && IsSaveLoadPanel(p);
}

void* SaveLoadFindLb(void* p) {
    // Engine member, not .gui id — a variant saveload.gui renumbers the
    // controls (same failure class as userlogs/077noequipment; see
    // docs/gui-id-audit.md).
    return acc::menus::detail::SaveLoadPanelGamesListBox(p);
}

// Speak on every step (no per-tick monitor watches the SaveLoad listbox).
// Pull planet/area from the row entry's CExoString fields directly — the
// preview labels at id=4/id=6 only refresh through the engine's
// onSelectionChanged callback that DriveListBoxSelection bypasses.
void SaveLoadAnnounce(void* /*lb*/, const ListBoxNavResult& r) {
    if (!r.row) return;
    char rowText[256];
    const char* rowSrc = acc::menus::extract::FromControl(
        r.row, rowText, sizeof(rowText));
    if (!rowSrc) return;
    const char* planet = ReadSaveLoadEntryString(
        r.row, kSaveLoadEntryLastModuleOffset);
    const char* area   = ReadSaveLoadEntryString(
        r.row, kSaveLoadEntryAreaNameOffset);
    char msg[512];
    if (planet || area) {
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtSaveLoadRow),
                 rowText, planet ? planet : "",
                 area ? area : "",
                 r.newSel + 1, r.rowCount);
    } else {
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtSaveLoadRowNoLoc),
                 rowText, r.newSel + 1, r.rowCount);
    }
    prism::Speak(msg, /*interrupt=*/false);
}

void SaveLoadLogExtra(char* out, size_t outN, const ListBoxNavResult& r) {
    const char* planet = ReadSaveLoadEntryString(
        r.row, kSaveLoadEntryLastModuleOffset);
    const char* area   = ReadSaveLoadEntryString(
        r.row, kSaveLoadEntryAreaNameOffset);
    snprintf(out, outN, " row=%p planet=\"%s\" area=\"%s\"",
             r.row, planet ? planet : "", area ? area : "");
}

bool SaveLoadOnEnter(void* panel) {
    // Arm the module-load latch when this is a LOAD (not a save). A save
    // keeps the world live and never changes the area, so the latch would
    // never clear; a load from the menu happens with no live player. Gate
    // on player-liveness: !GetPlayerPosition() ⇒ we're loading from the
    // main menu, so suppress the in-game-GUI reconstruction burst (the
    // engine activates every sub-screen panel as it rebuilds them, which
    // the menu focus hooks would otherwise narrate before the area-name
    // announce). The latch clears on the first fresh area pointer in
    // transitions::Tick. (Loading a save from *within* the game keeps the
    // world live here, so it isn't covered by this path.)
    Vector scratch{};
    if (!acc::engine::GetPlayerPosition(scratch)) {
        acc::transitions::NotifyExternalLoadStarting("SaveLoad load from menu");
    }
    acc::menus::detail::QueueControlActivate(
        acc::menus::detail::SaveLoadPanelActionButton(panel),
        "SaveLoad: Enter -> BTN_SAVELOAD");
    return true;
}

bool SaveLoadOnEsc(void* panel) {
    acc::menus::detail::QueueControlActivate(
        acc::menus::detail::SaveLoadPanelBackButton(panel),
        "SaveLoad: Esc -> BTN_BACK");
    return true;
}

}  // namespace

constexpr ListBoxPanelSpec kSaveLoadSpec = {
    /*logTag*/                  "SaveLoad",
    /*matches*/                 SaveLoadMatches,
    /*armed*/                   nullptr,
    /*resetStale*/              nullptr,
    /*findListBox*/             SaveLoadFindLb,
    /*minSel*/                  0,
    /*announce*/                SaveLoadAnnounce,
    /*enrichRow*/               nullptr,
    /*logExtra*/                SaveLoadLogExtra,
    /*onEnter*/                 SaveLoadOnEnter,
    /*onEsc*/                   SaveLoadOnEsc,
    /*titleOverride*/           nullptr,
    /*emptyStateId*/            acc::strings::Id::Count_,
    /*alwaysReturnFromHandler*/ false,  // fall through so chain-nav can reach Delete
};

// KOTOR 2 save/load: tell the engine which row the keyboard is on.
//
// K2 navigates this screen with the ordinary chain (rows and the five action
// buttons in one list), which is what the player expects and what makes Delete
// / Switch Characters / Cloud Saves reachable at all. But chain focus is OUR
// state: it moves and speaks the row while the engine's own selection index
// never changes, so Load loaded whatever was selected by default — the newest
// save, no matter which one had been announced. On K1 the mouse-teleport that
// accompanies a chain step lands on the row and the engine's hover-select
// follows it; on K2's layout the same teleport resolves to the list container
// or to nothing, so nothing ever propagates.
//
// Drive the engine's own SetSelectedControl from the chain's focused row —
// same call the listbox spec uses on K1, just sourced from chain focus instead
// of from a key press. Runs per tick rather than off the keypress because the
// chain step itself is deferred to the Update tick.
void MonitorK2SaveLoadSelection() {
    if (!acc::game::IsKotor2()) return;
    void* panel = acc::menus::chain::g_chainPanel;
    if (!panel || !IsSaveLoadPanel(panel)) return;

    void* lb = SaveLoadFindLb(panel);
    if (!lb) return;

    int idx = acc::menus::chain::g_chainIndex;
    if (idx < 0 || idx >= acc::menus::chain::kMaxChainEntries) return;
    void* focused = acc::menus::chain::g_chain[idx].control;
    if (!focused) return;

    // Which row of the listbox is focused? Chain focus is just as likely to be
    // one of the action buttons, in which case there is nothing to sync and
    // the last row selection must stand — that is the save Load will act on.
    int row = FindListBoxRowIndex(lb, focused);
    if (row < 0) return;

    static void* s_lastLb  = nullptr;
    static int   s_lastRow = -1;
    if (lb == s_lastLb && row == s_lastRow) return;
    s_lastLb  = lb;
    s_lastRow = row;

    if (!SetListBoxSelectedControl(lb, row)) return;
    acclog::Write("SaveLoad", "K2 chain sync: lb=%p row=%d control=%p",
                  lb, row, focused);

    // Announce the location on the NEXT tick, once the engine has repopulated
    // its preview labels from the selection we just set.
    s_pendingInfoPanel = panel;
}

// KOTOR 2 save/load: the per-slot location, one tick after the sync.
//
// K1 reads planet and area straight off the row object. That is not available
// here: a bounded scan of the K2 row for string fields (patch-20260813-214802)
// found nothing but coincidental pointer/length pairs, so the strings simply
// are not on the row — there are no offsets to port, and the probe that looked
// for them has been removed rather than left running.
//
// The panel's preview labels are the source instead. They are per-selection,
// not per-game, and they refresh through the engine's onSelectionChanged —
// which is precisely what the sync above now triggers, since it commits via
// the engine's own SetSelectedControl rather than a raw index write. Reading
// them a tick later gives the engine its frame to fill them in.
//
//   LBL_PLANETNAME   planet / module   ("Ebon Hawk")
//   LBL_AREANAME     area              ("Innenbereich")
//   LBL_TIMEPLAYED   play time         ("Zeit: 8S 16M")
//
// The labels are the panel's ctor-bound embedded members (gui-id audit;
// they were read by .gui id 3/5/9 before). LBL_TIMEPLAYED exists only on
// K2, and this whole function is K2-only.
//
// Play time is included where K1 does not, because K1's row text already
// carries it ("Game 1 - 21h 23m") and K2's does not — same information, from
// wherever each game happens to keep it.
void AnnounceK2SaveLoadInfo() {
    void* panel = s_pendingInfoPanel;
    if (!panel) return;
    s_pendingInfoPanel = nullptr;
    if (panel != acc::menus::chain::g_chainPanel) return;

    char planet[128] = "", area[128] = "", played[128] = "";
    void* c = acc::off::Ptr(panel, kSaveLoadPanelPlanetLabelOffset);
    if (c) acc::menus::extract::FromControl(c, planet, sizeof(planet), panel);
    c = acc::off::Ptr(panel, kSaveLoadPanelAreaLabelOffset);
    if (c) acc::menus::extract::FromControl(c, area, sizeof(area), panel);
    c = acc::off::Ptr(panel, kSaveLoadPanelTimeLabelOffset);
    if (c) acc::menus::extract::FromControl(c, played, sizeof(played), panel);

    char msg[384];
    msg[0] = '\0';
    const char* parts[3] = { planet, area, played };
    for (int i = 0; i < 3; ++i) {
        if (!parts[i][0]) continue;
        if (msg[0]) strncat_s(msg, ", ", _TRUNCATE);
        strncat_s(msg, parts[i], _TRUNCATE);
    }
    if (!msg[0]) return;

    // Channel 1 (the listbox-row channel): arrowing back onto a row whose
    // location was just spoken should not repeat it.
    acc::menus::SpeakIfChanged(/*channel=*/1, msg);
    acclog::Write("SaveLoad", "K2 slot info -> \"%s\"", msg);
}

// KOTOR 2 save/load: follow the Switch Characters button.
//
// Aspyr's screen keeps ONE visible list, filtered to a single character's
// saves (grouped by the savenfo PCNAME field). BTN_FILTER's engine handler
// advances a character index, destroys every row control, and rebuilds the
// list for the next character — which leaves the chain the user walks full
// of freed rows ("randomly one save per character", reported 2026-08-25).
// Nothing on the panel is per-row about WHO was switched to either; the
// engine writes the active character's name into LBL_PCNAME.
//
// So: watch the panel's character-filter index each tick. On a change,
// rebind the chain over the fresh rows and speak the new character's name
// plus how many saves the filtered list now holds. The engine has already
// re-selected the top (newest) row itself, so no selection sync is needed
// here — arrowing announces rows as normal afterwards.
//
// A row-set size change WITHOUT an index change is the Delete path — same
// stale-chain hazard, no announcement needed (the engine's own flow covers
// it); rebind preserving the cursor, same as the store monitor does on its
// list deltas.

// One SEH block for every raw engine read this monitor needs. Fills the
// filter index and the visible row count; false on fault or nonsense.
bool ReadK2SaveLoadSwapState(void* panel, void* lb, int* outIdx, int* outRows) {
    if (!panel || !lb) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(panel);
        int idx = *reinterpret_cast<int*>(
            base + kSaveLoadPanelCharFilterIndexOffset);
        int charCount = *reinterpret_cast<int*>(
            base + kSaveLoadPanelCharCountOffset);
        if (idx < 0 || charCount < 0 || charCount > 256 ||
            (charCount > 0 && idx >= charCount)) {
            return false;
        }
        auto* list = reinterpret_cast<CExoArrayList*>(
            reinterpret_cast<unsigned char*>(lb) + kListBoxControlsOffset);
        if (list->size < 0 || list->size > 4096) return false;
        *outIdx  = idx;
        *outRows = list->size;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void MonitorK2SaveLoadCharacterSwap() {
    if (!acc::game::IsKotor2()) return;

    static void* s_lastPanel = nullptr;
    static int   s_lastIdx   = -1;
    static int   s_lastRows  = -1;

    void* panel = acc::menus::chain::g_chainPanel;
    if (!panel || !IsSaveLoadPanel(panel)) {
        s_lastPanel = nullptr;
        return;
    }
    void* lb = SaveLoadFindLb(panel);
    int idx = -1, rows = -1;
    if (!lb || !ReadK2SaveLoadSwapState(panel, lb, &idx, &rows)) return;

    if (panel != s_lastPanel) {
        // First sight of this panel visit: prime only. Entry speech stays
        // title + focus; the default character is not announced.
        s_lastPanel = panel;
        s_lastIdx   = idx;
        s_lastRows  = rows;
        return;
    }

    if (idx != s_lastIdx) {
        s_lastIdx  = idx;
        s_lastRows = rows;
        acc::menus::chain::RebindChain(panel);
        char name[128] = "";
        void* c = acc::off::Ptr(panel, kSaveLoadPanelPcNameLabelOffset);
        if (c) acc::menus::extract::FromControl(c, name, sizeof(name), panel);
        char msg[192];
        snprintf(msg, sizeof(msg),
                 acc::strings::Get(acc::strings::Id::FmtSaveLoadCharacter),
                 name, rows);
        prism::Speak(msg, /*interrupt=*/false);
        acclog::Write("SaveLoad",
                      "K2 character swap -> idx=%d rows=%d name=\"%s\"; chain rebound",
                      idx, rows, name);
        return;
    }

    if (rows != s_lastRows) {
        acclog::Write("SaveLoad",
                      "K2 row set %d -> %d (same character); chain rebind preserving idx",
                      s_lastRows, rows);
        s_lastRows = rows;
        acc::menus::chain::RebindChainPreserveIndex(panel);
    }
}

}  // namespace acc::menus::listbox
