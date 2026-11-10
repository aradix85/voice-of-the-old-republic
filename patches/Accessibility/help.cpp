// Help system — see help.h for the design summary.
//
// One catalog (kEntries) feeds both surfaces. `grp` decides the F1 section a
// line is listed under (F1 lists every entry); `ctx` is a curated bitmask of
// the screens that speak the line on Ctrl+F1 (kept short on purpose). Adding a
// keybind = add a strings::Id (strings.h) + one row here.

#include "help.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "engine_game.h"        // IsKotor2 — gates the K2-only key rows
#include "engine_panels.h"
#include "engine_player.h"      // GetPlayerPosition / Vector
#include "engine_subscreen.h"   // Begin/EndOverlayPause
#include "hotkeys.h"
                                // so the engine-event consume guard wins the
                                // poll-vs-event race (no stray Options open)
#include "input_pipeline.h"     // NoteOverlayEscClosed — latch the Esc-close
#include "log.h"
#include "minigame_pazaak.h"   // IsBoardForeground — the pazaak help context
#include "speech.h"
#include "strings.h"
#include "unified_action_menu.h"

namespace acc::help {

namespace {

using S = acc::strings::Id;

// ----- Context model ------------------------------------------------------
// Bits used by both the ctx tags below and Ctrl+F1 selection.
enum CtxBit : uint32_t {
    kWorld      = 1u << 0,
    kMenu       = 1u << 1,
    kMap        = 1u << 2,
    kActionMenu = 1u << 3,
    kDialog     = 1u << 4,
    kContainer  = 1u << 5,
    kStore      = 1u << 6,
    kPazaak     = 1u << 7,
};

enum class Context { World, Menu, Map, ActionMenu, Dialog, Container, Store,
                     Pazaak };

uint32_t ContextBit(Context c) {
    switch (c) {
        case Context::World:      return kWorld;
        case Context::Menu:       return kMenu;
        case Context::Map:        return kMap;
        case Context::ActionMenu: return kActionMenu;
        case Context::Dialog:     return kDialog;
        case Context::Container:  return kContainer;
        case Context::Store:      return kStore;
        case Context::Pazaak:     return kPazaak;
    }
    return kWorld;
}

S ContextNameId(Context c) {
    switch (c) {
        case Context::World:      return S::HelpContextWorld;
        case Context::Menu:       return S::HelpContextMenu;
        case Context::Map:        return S::HelpContextMap;
        case Context::ActionMenu: return S::HelpContextActionMenu;
        case Context::Dialog:     return S::HelpContextDialog;
        case Context::Container:  return S::HelpContextContainer;
        case Context::Store:      return S::HelpContextStore;
        case Context::Pazaak:     return S::HelpContextPazaak;
    }
    return S::HelpContextWorld;
}

// Detect the screen the user is on. Priority order: the most specific
// surfaces first, the generic "some menu is up" next, pure world last.
Context DetectContext() {
    // The pazaak board first: it is a foreground panel, so every check below
    // would call it a generic Menu and answer with menu keys the minigame
    // does not use. Its own arrow-zone navigator and letter shortcuts are the
    // only keys that do anything while it is up.
    if (acc::pazaak::IsBoardForeground()) return Context::Pazaak;

    if (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) {
        return Context::ActionMenu;
    }
    if (acc::engine::HasActiveDialogPanel()) return Context::Dialog;

    acc::engine::UiBlockState st;
    bool blocked = acc::engine::IsForegroundUiBlocking(&st);
    if (st.fgKind == acc::engine::PanelKind::Container) return Context::Container;
    if (st.fgKind == acc::engine::PanelKind::Store)     return Context::Store;
    if (acc::engine::HasActiveMapPanel(nullptr))        return Context::Map;
    if (blocked) return Context::Menu;

    Vector p;
    if (acc::engine::GetPlayerPosition(p)) return Context::World;
    return Context::Menu;  // title screen / loading — no world, no block
}

// ----- Catalog ------------------------------------------------------------
enum class Grp {
    General, Movement, Interaction, Combat, Exploration, Screens, Map, Pazaak,
    Mod,
    COUNT
};

constexpr S kGroupHeader[] = {
    S::HelpGroupGeneral, S::HelpGroupMovement, S::HelpGroupInteraction,
    S::HelpGroupCombat, S::HelpGroupExploration, S::HelpGroupScreens,
    S::HelpGroupMap, S::HelpGroupPazaak, S::HelpGroupMod,
};
static_assert(sizeof(kGroupHeader) / sizeof(kGroupHeader[0]) ==
              static_cast<int>(Grp::COUNT),
              "kGroupHeader must cover every Grp");

struct Entry {
    S        label;     // header/simple-entry id, or (composed) the format id
    Grp      grp;
    uint32_t ctx;       // 0 = F1-list only, never spoken by Ctrl+F1
    bool     composed;  // true → label is a format; text built from MenuCat*
                        // names at BuildRows time (the number-key line)
    bool     k2Only;    // true → listed only on KOTOR 2. For bindings that
                        // exist because the second game has something the
                        // first does not (its fifth action-bar column), where
                        // listing the key on KOTOR 1 would promise a category
                        // that is never populated there.
};

// Order within a group is the order F1 reads them. The General nav keys are
// tagged for the menu-like screens they apply to (not World) so they're heard
// once, not repeated per section.
constexpr Entry kEntries[] = {
    // ---- General navigation ----
    { S::HelpKeyUpDown,          Grp::General, kMenu | kActionMenu | kDialog | kContainer | kStore },
    { S::HelpKeyLeftRight,       Grp::General, kMenu | kActionMenu },
    { S::HelpKeyHomeEnd,         Grp::General, kMenu | kActionMenu },
    { S::HelpKeyEnter,           Grp::General, kMenu | kActionMenu | kDialog | kContainer | kStore },
    { S::HelpKeyEsc,             Grp::General, kMenu | kMap | kActionMenu | kDialog | kContainer | kStore },
    { S::HelpKeyReadDescription, Grp::General, kMenu | kActionMenu },
    // Q/E — engine window/tab/screen cycling plus our store buy-sell and
    // container take-give mode toggles, all under one line.
    { S::HelpKeySwitchWindows,   Grp::General, kMenu | kContainer | kStore },
    // F1 / Ctrl+F1 are listed in the F1 menu but never spoken by Ctrl+F1
    // itself — a user who reached Ctrl+F1 has already found F1.
    { S::HelpKeyF1,              Grp::General, 0 },
    { S::HelpKeyCtrlF1,          Grp::General, 0 },

    // ---- Movement & camera ----
    { S::HelpKeyWalk,            Grp::Movement, kWorld },
    { S::HelpKeyCameraRotate,    Grp::Movement, kWorld },
    { S::HelpKeyStrafe,          Grp::Movement, 0 },
    { S::HelpKeyPause,           Grp::Movement, 0 },
    { S::HelpKeyViewMode,        Grp::Movement, 0 },
    { S::HelpKeySwitchLeader,    Grp::Movement, 0 },

    // ---- Targeting & interaction ----
    { S::HelpKeyCycleTargets,    Grp::Interaction, kWorld },
    { S::HelpKeyInteract,        Grp::Interaction, kWorld },
    { S::HelpKeyOpenActionMenu,  Grp::Interaction, kWorld },
    { S::HelpKeySelfStatus,      Grp::Interaction, kWorld },
    { S::HelpKeyPartyStatus,     Grp::Interaction, kWorld },
    { S::HelpKeyAnnounceFocus,   Grp::Interaction, kWorld },
    { S::HelpKeyWalkToFocus,     Grp::Interaction, 0 },
    { S::HelpKeyRoute,          Grp::Interaction, 0 },
    { S::HelpKeyDialogRepeat,    Grp::Interaction, kDialog },

    // ---- Exploration & orientation ----
    { S::HelpKeyCycleObjects,    Grp::Exploration, 0 },
    { S::HelpKeyCycleCategory,   Grp::Exploration, 0 },
    { S::HelpKeyCycleEnds,       Grp::Exploration, 0 },
    { S::HelpKeyHeading,         Grp::Exploration, 0 },
    { S::HelpKeyCameraOrient,    Grp::Exploration, 0 },
    { S::HelpKeyExploreOverview, Grp::Exploration, kWorld },
    { S::HelpKeyExploreWalk,     Grp::Exploration, kWorld },
    { S::HelpKeyDropMarker,      Grp::Exploration, kMap },

    // ---- Combat & actions ----
    // Bare 1..7 fire a category's most recent action; Shift+1..7 open the
    // category. The bare-key line is composed from the MenuCat* names so it
    // matches what the action menu speaks (1 Attacks .. 6 Misc, 7 Explosives;
    // linear key→column order, matching the engine dispatch).
    { S::FmtHelpNumberActions,   Grp::Combat, 0, /*composed=*/true },
    { S::HelpKeyOpenCategory,    Grp::Combat, 0 },
    // Key 8 is its own line rather than an eighth slot in the composed 1..7
    // one, because it exists only on KOTOR 2 — folding it in would need two
    // variants of that format string in seven languages, and would list a
    // category KOTOR 1 never populates.
    { S::FmtHelpNumberActions8,  Grp::Combat, 0, /*composed=*/true,
      /*k2Only=*/true },
    // Key 9 likewise: KOTOR 2's forms column.
    { S::FmtHelpNumberActions9,  Grp::Combat, 0, /*composed=*/true,
      /*k2Only=*/true },
    { S::HelpKeyActionQueue,     Grp::Combat, 0 },
    { S::HelpKeyLevelUp,         Grp::Combat, 0 },
    { S::HelpKeyCancelCombat,    Grp::Combat, 0 },

    // ---- Quick screens ----
    { S::HelpKeyScreenMap,       Grp::Screens, 0 },
    { S::HelpKeyScreenMessages,  Grp::Screens, 0 },
    { S::HelpKeyScreenQuests,    Grp::Screens, 0 },
    { S::HelpKeyScreenAbilities, Grp::Screens, 0 },
    { S::HelpKeyScreenCharacter, Grp::Screens, 0 },
    { S::HelpKeyScreenInventory, Grp::Screens, 0 },
    { S::HelpKeyScreenEquip,     Grp::Screens, 0 },
    { S::HelpKeyScreenOptions,   Grp::Screens, 0 },

    // ---- Map panel ----
    { S::HelpKeyMapCursor,       Grp::Map, kMap },
    { S::HelpKeyMapPosition,     Grp::Map, kMap },

    // ---- Pazaak board ----
    // Tagged for kPazaak only. The board is a foreground panel but not a
    // navigable menu: our own navigator owns the arrows and Enter, and the
    // generic General lines (Q/E windows, Shift+arrow description) do nothing
    // there — so Ctrl+F1 on the board answers with these four and nothing
    // else. The side-deck builder and the wager popup are ordinary chain
    // screens and keep the Menu context.
    { S::HelpKeyPazaakNav,       Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakSign,      Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakReview,    Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakTurn,      Grp::Pazaak, kPazaak },

    // ---- Mod features ----
    // F1-list only — Ctrl+F1 deliberately doesn't mention mod settings.
    { S::HelpKeyModSettings,     Grp::Mod, 0 },
    // Ctrl+R works on every screen, but Ctrl+F1 only volunteers it where a
    // player actually wants text out of the game: a dialog line (the
    // number-heavy terminal riddles), a menu entry, an object announcement.
    { S::HelpKeyCopyLastSpoken,  Grp::Mod, kWorld | kDialog | kMenu },

};
constexpr int kEntryCount =
    static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));

// ----- F1 list overlay state ----------------------------------------------
constexpr int kMaxRows = 96;  // 8 headers + entries, with headroom

struct Row {
    bool isHeader;
    bool composed;     // entry text pre-built into text[] (number-key line)
    S    label;        // header id, or simple-entry id
    char text[224];    // composed entry text (only when composed)
    int  entryPos;     // 1-based position among entries (headers: 0)
};

struct State {
    bool open        = false;
    bool pausedWorld = false;  // BeginOverlayPause held (in-world open only)
    int  focus       = 0;
    int  rowCount    = 0;
    int  entryTotal  = 0;
    Row  rows[kMaxRows];
};
State g_state;

// Resolve a composed entry's text. The bare-number line lists the category
// names (1..7) using the same MenuCat* strings the action menu speaks, so the
// help never drifts from what the user navigates. Key→category order is linear
// and matches the engine dispatch: 6 → Misc (Sonstiges), 7 → Explosives.
void BuildComposedText(S formatId, char* out, size_t cap) {
    if (formatId == S::FmtHelpNumberActions) {
        std::snprintf(out, cap, acc::strings::Get(formatId),
                      acc::strings::Get(S::MenuCatAttacks),
                      acc::strings::Get(S::MenuCatForcePowers),
                      acc::strings::Get(S::MenuCatItems),
                      acc::strings::Get(S::MenuCatSelfPowers),
                      acc::strings::Get(S::MenuCatMedical),
                      acc::strings::Get(S::MenuCatMisc),
                      acc::strings::Get(S::MenuCatExplosives));
        return;
    }
    if (formatId == S::FmtHelpNumberActions8) {
        std::snprintf(out, cap, acc::strings::Get(formatId),
                      acc::strings::Get(S::MenuCatCombatBehaviour));
        return;
    }
    if (formatId == S::FmtHelpNumberActions9) {
        std::snprintf(out, cap, acc::strings::Get(formatId),
                      acc::strings::Get(S::MenuCatForms));
        return;
    }
    std::snprintf(out, cap, "%s", acc::strings::Get(formatId));
}

// Build the flat row list: each non-empty group emits a header row followed
// by its entries, in catalog order.
// A row is listed unless it is for KOTOR 2 and this is KOTOR 1.
bool EntryListed(const Entry& e) {
    return !e.k2Only || acc::game::IsKotor2();
}

void BuildRows() {
    const int idxLimit = kMaxRows;
    int idx = 0;
    int entryNo = 0;
    for (int g = 0; g < static_cast<int>(Grp::COUNT) && idx < idxLimit; ++g) {
        // Count this group's listed entries first so a group with nothing to
        // show is skipped rather than emitting a lone header.
        bool any = false;
        for (int e = 0; e < kEntryCount; ++e) {
            if (kEntries[e].grp == static_cast<Grp>(g) &&
                EntryListed(kEntries[e])) { any = true; break; }
        }
        if (!any) continue;

        Row& hdr = g_state.rows[idx];
        hdr.isHeader = true;
        hdr.composed = false;
        hdr.label    = kGroupHeader[g];
        hdr.text[0]  = '\0';
        hdr.entryPos = 0;
        ++idx;

        for (int e = 0; e < kEntryCount && idx < idxLimit; ++e) {
            if (kEntries[e].grp != static_cast<Grp>(g)) continue;
            if (!EntryListed(kEntries[e])) continue;
            ++entryNo;
            Row& row = g_state.rows[idx];
            row.isHeader = false;
            row.composed = kEntries[e].composed;
            row.label    = kEntries[e].label;
            row.entryPos = entryNo;
            row.text[0]  = '\0';
            if (row.composed) {
                BuildComposedText(kEntries[e].label, row.text, sizeof(row.text));
            }
            ++idx;
        }
    }
    g_state.rowCount   = idx;
    g_state.entryTotal = entryNo;
}

void SpeakRow(int idx, bool interrupt) {
    if (idx < 0 || idx >= g_state.rowCount) return;
    const Row& r = g_state.rows[idx];
    char line[384];
    if (r.isHeader) {
        std::snprintf(line, sizeof(line),
                      acc::strings::Get(S::FmtHelpGroupHeader),
                      acc::strings::Get(r.label));
    } else {
        const char* base = r.composed ? r.text : acc::strings::Get(r.label);
        std::snprintf(line, sizeof(line),
                      acc::strings::Get(S::FmtHelpRowOf),
                      base, r.entryPos, g_state.entryTotal);
    }
    acc::speech::Speak(line, interrupt);
}

// Focus movement. Both clamp (no wrap) — the repeated line is the
// boundary cue, same rule the mod's submenus follow.
void StepFocus(int delta) {
    const int next = g_state.focus + delta;
    if (next >= 0 && next < g_state.rowCount) g_state.focus = next;
    SpeakRow(g_state.focus, /*interrupt=*/true);
}

void FocusEdge(bool last) {
    g_state.focus = (last && g_state.rowCount > 0) ? g_state.rowCount - 1 : 0;
    SpeakRow(g_state.focus, /*interrupt=*/true);
}

// Ctrl+F1 — speak the keys tagged for the current screen, joined.
void SpeakContext() {
    Context c   = DetectContext();
    uint32_t bit = ContextBit(c);

    char joined[1024];
    joined[0] = '\0';
    size_t len = 0;
    int n = 0;
    for (int e = 0; e < kEntryCount; ++e) {
        if (!(kEntries[e].ctx & bit)) continue;
        if (!EntryListed(kEntries[e])) continue;
        const char* t = acc::strings::Get(kEntries[e].label);
        if (!t[0]) continue;
        if (n > 0) {
            // Separator between entries.
            len += static_cast<size_t>(
                std::snprintf(joined + len, sizeof(joined) - len, ". "));
            if (len >= sizeof(joined) - 1) break;
        }
        len += static_cast<size_t>(
            std::snprintf(joined + len, sizeof(joined) - len, "%s", t));
        ++n;
        if (len >= sizeof(joined) - 1) break;
    }

    char out[1280];
    if (n == 0) {
        std::snprintf(out, sizeof(out), "%s",
                      acc::strings::Get(S::HelpContextNothing));
    } else {
        std::snprintf(out, sizeof(out),
                      acc::strings::Get(S::FmtHelpContextLine),
                      acc::strings::Get(ContextNameId(c)),
                      joined);
    }
    acc::speech::Speak(out, /*interrupt=*/true);
    acclog::Write("Help", "context speak ctx=%d entries=%d", static_cast<int>(c), n);
}

}  // namespace

bool IsMenuOpen() { return g_state.open; }

void OpenMenu() {
    if (g_state.open) return;
    BuildRows();
    g_state.focus = 0;
    g_state.open  = true;

    // Pause the world only when opened from pure in-world context — in a menu
    // the engine panel already holds the world, and BeginOverlayPause routes
    // through the server-side combat pause which has no meaning there.
    g_state.pausedWorld = (DetectContext() == Context::World);
    if (g_state.pausedWorld)
        acc::engine::BeginOverlayPause(acc::engine::OverlayPauseOwner::Help);

    acclog::Write("Help", "menu opened rows=%d entries=%d pausedWorld=%d",
                  g_state.rowCount, g_state.entryTotal,
                  g_state.pausedWorld ? 1 : 0);

    acc::speech::Speak(acc::strings::Get(S::HelpMenuOpened), /*interrupt=*/true);
    SpeakRow(g_state.focus, /*interrupt=*/false);
}

void CloseMenu() {
    if (!g_state.open) return;
    acclog::Write("Help", "menu closed");
    g_state.open = false;
    if (g_state.pausedWorld) {
        acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::Help);
        g_state.pausedWorld = false;
    }
    g_state.focus    = 0;
    g_state.rowCount = 0;
    acc::speech::Speak(acc::strings::Get(S::HelpMenuClosed), /*interrupt=*/true);
}

void ToggleMenu() {
    if (g_state.open) CloseMenu();
    else              OpenMenu();
}

void PollWin32() {
    using A = acc::hotkeys::Action;

    // Ctrl+F1 — context help. Independent of the list being open.
    if (acc::hotkeys::Pressed(A::HelpContext)) {
        SpeakContext();
    }

    // F1 — toggle the list. Consume so the press can't leak to any other
    // consumer (and the manager hook separately suppresses the engine's
    // F1-as-activate when in a menu).
    if (acc::hotkeys::Pressed(A::HelpMenuOpen)) {
        ToggleMenu();
        acc::hotkeys::Consume(A::HelpMenuOpen);
    }

    if (!g_state.open) return;

    // List navigation. Pure-linear Up/Down with Home/End; Enter re-reads;
    // Esc closes. Each handled key is Consume()d so the downstream in-world
    // pollers (cycle / interact / examine) and the Home/End synthesiser don't
    // also act on it this tick.
    if (acc::hotkeys::Pressed(A::NavUp)) {
        StepFocus(-1);
        acc::hotkeys::Consume(A::NavUp);
    }
    if (acc::hotkeys::Pressed(A::NavDown)) {
        StepFocus(+1);
        acc::hotkeys::Consume(A::NavDown);
    }
    if (acc::hotkeys::Pressed(A::NavHome)) {
        FocusEdge(/*last=*/false);
        acc::hotkeys::Consume(A::NavHome);
    }
    if (acc::hotkeys::Pressed(A::NavEnd)) {
        FocusEdge(/*last=*/true);
        acc::hotkeys::Consume(A::NavEnd);
    }
    if (acc::hotkeys::Pressed(A::InteractTarget)) {  // Enter — re-read
        SpeakRow(g_state.focus, /*interrupt=*/true);
        acc::hotkeys::Consume(A::InteractTarget);
    }
    if (acc::hotkeys::Pressed(A::SubmenuEsc)) {
        CloseMenu();
        acc::input::NoteOverlayEscClosed();
        acc::hotkeys::Consume(A::SubmenuEsc);
    }
}

void Tick() {
    if (!g_state.open) return;
    // Self-disarm: an in-world open whose world drops out (area load /
    // teardown) must release the pause hold and close, or the overlay
    // strands a paused world across the transition.
    if (g_state.pausedWorld) {
        Vector p;
        if (!acc::engine::GetPlayerPosition(p)) {
            acclog::Write("Help", "self-disarm: player position lost");
            // Close without the "closed" speech — the transition has its own
            // announce path and we don't want to step on it.
            g_state.open = false;
            acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::Help);
            g_state.pausedWorld = false;
            g_state.focus    = 0;
            g_state.rowCount = 0;
        }
    }
}

}  // namespace acc::help
