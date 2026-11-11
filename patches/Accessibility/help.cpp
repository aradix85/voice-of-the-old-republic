// The help system; help.h has the design. One catalog (kEntries) feeds both
// surfaces: `grp` is the F1 section a line is listed under (F1 lists every
// entry), `ctx` the screens that speak the line on Ctrl+F1 (kept short on
// purpose). A new keybind is a strings::Id (strings.h) and one row here.

#include "help.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "engine_game.h"       // IsKotor2: the KOTOR 2-only key rows
#include "engine_panels.h"
#include "engine_player.h"
#include "engine_subscreen.h"  // Begin/EndOverlayPause
#include "hotkeys.h"
#include "input_pipeline.h"    // NoteOverlayEscClosed
#include "log.h"
#include "minigame_pazaak.h"   // IsBoardForeground
#include "speech.h"
#include "strings.h"
#include "unified_action_menu.h"

namespace acc::help {

namespace {

using S = acc::strings::Id;

const char* Str(S id) { return acc::strings::Get(id); }

// A screen, as a bit: an entry's `ctx` is a mask of them.
enum Context : uint32_t {
    kWorld      = 1u << 0,
    kMenu       = 1u << 1,
    kMap        = 1u << 2,
    kActionMenu = 1u << 3,
    kDialog     = 1u << 4,
    kContainer  = 1u << 5,
    kStore      = 1u << 6,
    kPazaak     = 1u << 7,
};

S ContextName(Context c) {
    switch (c) {
        case kWorld:      return S::HelpContextWorld;
        case kMenu:       return S::HelpContextMenu;
        case kMap:        return S::HelpContextMap;
        case kActionMenu: return S::HelpContextActionMenu;
        case kDialog:     return S::HelpContextDialog;
        case kContainer:  return S::HelpContextContainer;
        case kStore:      return S::HelpContextStore;
        case kPazaak:     return S::HelpContextPazaak;
    }
    return S::HelpContextWorld;
}

// The screen the user is on: the most specific surfaces first, then "some
// menu is up", the world last.
Context DetectContext() {
    // The pazaak board is a foreground panel, so every check below would call
    // it a menu and answer with menu keys the minigame does not use.
    if (acc::pazaak::IsBoardForeground()) return kPazaak;

    if (acc::unified_menu::IsActive() && !acc::unified_menu::IsSuspended()) {
        return kActionMenu;
    }
    if (acc::engine::HasActiveDialogPanel()) return kDialog;

    acc::engine::UiBlockState st;
    const bool blocked = acc::engine::IsForegroundUiBlocking(&st);
    if (st.fgKind == acc::engine::PanelKind::Container) return kContainer;
    if (st.fgKind == acc::engine::PanelKind::Store)     return kStore;
    if (acc::engine::HasActiveMapPanel(nullptr))        return kMap;
    if (blocked) return kMenu;

    Vector p;
    if (acc::engine::GetPlayerPosition(p)) return kWorld;
    return kMenu;  // title screen or loading: no world and no blocking panel
}

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
    S        label;   // the line, or its format (see EntryText)
    Grp      grp;
    uint32_t ctx;     // 0 = listed by F1 only, never spoken by Ctrl+F1
    bool     k2Only;  // a binding for something KOTOR 1 does not have
};

// The screens with a list to walk.
constexpr uint32_t kLists = kMenu | kActionMenu | kDialog | kContainer | kStore;

// Order within a group is the order F1 reads them. The General navigation
// keys are tagged for the menu-like screens they apply to, not for the world,
// so that they are heard once.
constexpr Entry kEntries[] = {
    { S::HelpKeyUpDown,          Grp::General, kLists },
    { S::HelpKeyLeftRight,       Grp::General, kMenu | kActionMenu },
    { S::HelpKeyHomeEnd,         Grp::General, kMenu | kActionMenu },
    { S::HelpKeyEnter,           Grp::General, kLists },
    { S::HelpKeyEsc,             Grp::General, kLists | kMap },
    { S::HelpKeyReadDescription, Grp::General, kMenu | kActionMenu },
    // Q/E: the engine's window, tab and screen cycling, and our buy/sell and
    // take/give mode toggles.
    { S::HelpKeySwitchWindows,   Grp::General, kMenu | kContainer | kStore },
    // Who reached Ctrl+F1 has already found F1.
    { S::HelpKeyF1,              Grp::General, 0 },
    { S::HelpKeyCtrlF1,          Grp::General, 0 },

    { S::HelpKeyWalk,            Grp::Movement, kWorld },
    { S::HelpKeyCameraRotate,    Grp::Movement, kWorld },
    { S::HelpKeyStrafe,          Grp::Movement, 0 },
    { S::HelpKeyPause,           Grp::Movement, 0 },
    { S::HelpKeySwitchLeader,    Grp::Movement, 0 },

    { S::HelpKeyCycleTargets,    Grp::Interaction, kWorld },
    { S::HelpKeyInteract,        Grp::Interaction, kWorld },
    { S::HelpKeyOpenActionMenu,  Grp::Interaction, kWorld },
    { S::HelpKeySelfStatus,      Grp::Interaction, kWorld },
    { S::HelpKeyPartyStatus,     Grp::Interaction, kWorld },
    { S::HelpKeyAnnounceFocus,   Grp::Interaction, kWorld },
    { S::HelpKeyWalkToFocus,     Grp::Interaction, 0 },
    { S::HelpKeyRoute,           Grp::Interaction, 0 },
    { S::HelpKeyDialogRepeat,    Grp::Interaction, kDialog },

    { S::HelpKeyCycleObjects,    Grp::Exploration, 0 },
    { S::HelpKeyCycleCategory,   Grp::Exploration, 0 },
    { S::HelpKeyCycleEnds,       Grp::Exploration, 0 },
    { S::HelpKeyHeading,         Grp::Exploration, 0 },
    { S::HelpKeyCameraOrient,    Grp::Exploration, 0 },
    { S::HelpKeyExploreOverview, Grp::Exploration, kWorld },
    { S::HelpKeyExploreWalk,     Grp::Exploration, kWorld },
    { S::HelpKeyDropMarker,      Grp::Exploration, kMap },

    // Bare 1..7 fire a category's most recent action, Shift+1..7 open the
    // category. Keys 8 and 9 are lines of their own: their columns (combat
    // behaviour, forms) exist on KOTOR 2 only.
    { S::FmtHelpNumberActions,   Grp::Combat, 0 },
    { S::HelpKeyOpenCategory,    Grp::Combat, 0 },
    { S::FmtHelpNumberActions8,  Grp::Combat, 0, /*k2Only=*/true },
    { S::FmtHelpNumberActions9,  Grp::Combat, 0, /*k2Only=*/true },
    { S::HelpKeyActionQueue,     Grp::Combat, 0 },
    { S::HelpKeyLevelUp,         Grp::Combat, 0 },
    { S::HelpKeyCancelCombat,    Grp::Combat, 0 },

    { S::HelpKeyScreenMap,       Grp::Screens, 0 },
    { S::HelpKeyScreenMessages,  Grp::Screens, 0 },
    { S::HelpKeyScreenQuests,    Grp::Screens, 0 },
    { S::HelpKeyScreenAbilities, Grp::Screens, 0 },
    { S::HelpKeyScreenCharacter, Grp::Screens, 0 },
    { S::HelpKeyScreenInventory, Grp::Screens, 0 },
    { S::HelpKeyScreenEquip,     Grp::Screens, 0 },
    { S::HelpKeyScreenOptions,   Grp::Screens, 0 },

    { S::HelpKeyMapCursor,       Grp::Map, kMap },
    { S::HelpKeyMapPosition,     Grp::Map, kMap },

    // The board is a foreground panel but no navigable menu: our navigator
    // owns the arrows and Enter, and the General lines do nothing there. The
    // side-deck builder and the wager popup are ordinary menus.
    { S::HelpKeyPazaakNav,       Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakSign,      Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakReview,    Grp::Pazaak, kPazaak },
    { S::HelpKeyPazaakTurn,      Grp::Pazaak, kPazaak },

    { S::HelpKeyModSettings,     Grp::Mod, 0 },
    // Ctrl+R works everywhere; Ctrl+F1 offers it where a player wants text
    // out of the game: a dialog line, a menu entry, an object announcement.
    { S::HelpKeyCopyLastSpoken,  Grp::Mod, kWorld | kDialog | kMenu },
};
constexpr int kEntryCount =
    static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));

// A line that KOTOR 2 alone has is not listed on KOTOR 1.
bool EntryListed(const Entry& e) {
    return !e.k2Only || acc::game::IsKotor2();
}

// An entry's line. The number-key lines are formats over the MenuCat* names,
// so that help names the categories as the action menu speaks them, in the
// engine's key order (6 is Miscellaneous, 7 Explosives).
void EntryText(S label, char* out, size_t cap) {
    if (label == S::FmtHelpNumberActions) {
        std::snprintf(out, cap, Str(label),
                      Str(S::MenuCatAttacks), Str(S::MenuCatForcePowers),
                      Str(S::MenuCatItems), Str(S::MenuCatSelfPowers),
                      Str(S::MenuCatMedical), Str(S::MenuCatMisc),
                      Str(S::MenuCatExplosives));
    } else if (label == S::FmtHelpNumberActions8) {
        std::snprintf(out, cap, Str(label), Str(S::MenuCatCombatBehaviour));
    } else if (label == S::FmtHelpNumberActions9) {
        std::snprintf(out, cap, Str(label), Str(S::MenuCatForms));
    } else {
        std::snprintf(out, cap, "%s", Str(label));
    }
}

constexpr size_t kEntryTextCap = 224;

// The F1 list: a header row per group that has entries, then its entries.
struct Row {
    bool isHeader;
    S    label;
    int  entryPos;  // 1-based place among the entries; 0 for a header
};

constexpr int kMaxRows = kEntryCount + static_cast<int>(Grp::COUNT);

struct State {
    bool open        = false;
    bool pausedWorld = false;  // holds the overlay pause (opened in the world)
    int  focus       = 0;
    int  rowCount    = 0;
    int  entryTotal  = 0;
    Row  rows[kMaxRows];
};
State g_state;

void BuildRows() {
    int rowCount = 0;
    int entryNo = 0;
    for (int g = 0; g < static_cast<int>(Grp::COUNT); ++g) {
        bool headed = false;
        for (const Entry& e : kEntries) {
            if (e.grp != static_cast<Grp>(g) || !EntryListed(e)) continue;
            if (!headed) {
                g_state.rows[rowCount++] = { true, kGroupHeader[g], 0 };
                headed = true;
            }
            g_state.rows[rowCount++] = { false, e.label, ++entryNo };
        }
    }
    g_state.rowCount   = rowCount;
    g_state.entryTotal = entryNo;
}

void SpeakRow(int idx, bool interrupt) {
    if (idx < 0 || idx >= g_state.rowCount) return;
    const Row& r = g_state.rows[idx];
    char line[384];
    if (r.isHeader) {
        std::snprintf(line, sizeof(line), Str(S::FmtHelpGroupHeader), Str(r.label));
    } else {
        char text[kEntryTextCap];
        EntryText(r.label, text, sizeof(text));
        std::snprintf(line, sizeof(line), Str(S::FmtHelpRowOf),
                      text, r.entryPos, g_state.entryTotal);
    }
    acc::speech::Speak(line, interrupt);
}

// Clamped, no wrap: the repeated line is the boundary cue.
void FocusRow(int idx) {
    if (g_state.rowCount <= 0) return;
    g_state.focus = std::clamp(idx, 0, g_state.rowCount - 1);
    SpeakRow(g_state.focus, /*interrupt=*/true);
}

// Ctrl+F1: the lines tagged for the current screen, joined.
void SpeakContext() {
    const Context c = DetectContext();

    char joined[1024] = "";
    size_t len = 0;
    int n = 0;
    for (const Entry& e : kEntries) {
        if (!(e.ctx & c) || !EntryListed(e)) continue;
        char text[kEntryTextCap];
        EntryText(e.label, text, sizeof(text));
        if (!text[0]) continue;
        len += static_cast<size_t>(std::snprintf(
            joined + len, sizeof(joined) - len, "%s%s", n > 0 ? ". " : "", text));
        ++n;
        if (len >= sizeof(joined) - 1) break;  // full: the rest is cut
    }

    char out[1280];
    if (n == 0) {
        std::snprintf(out, sizeof(out), "%s", Str(S::HelpContextNothing));
    } else {
        std::snprintf(out, sizeof(out), Str(S::FmtHelpContextLine),
                      Str(ContextName(c)), joined);
    }
    acc::speech::Speak(out, /*interrupt=*/true);
    acclog::Write("Help", "context speak ctx=0x%x entries=%d",
                  static_cast<unsigned>(c), n);
}

void Open() {
    BuildRows();
    g_state.focus = 0;
    g_state.open  = true;

    // The world is paused only when the list opens in the world: in a menu
    // the engine panel already holds it, and BeginOverlayPause goes through
    // the server-side combat pause, which means nothing there.
    g_state.pausedWorld = DetectContext() == kWorld;
    if (g_state.pausedWorld) {
        acc::engine::BeginOverlayPause(acc::engine::OverlayPauseOwner::Help);
    }
    acclog::Write("Help", "menu opened rows=%d entries=%d pausedWorld=%d",
                  g_state.rowCount, g_state.entryTotal,
                  g_state.pausedWorld ? 1 : 0);

    acc::speech::Speak(Str(S::HelpMenuOpened), /*interrupt=*/true);
    SpeakRow(g_state.focus, /*interrupt=*/false);
}

void Close(bool announce) {
    g_state.open = false;
    if (g_state.pausedWorld) {
        acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::Help);
        g_state.pausedWorld = false;
    }
    g_state.focus    = 0;
    g_state.rowCount = 0;
    if (announce) acc::speech::Speak(Str(S::HelpMenuClosed), /*interrupt=*/true);
}

// This tick's press of `a`, claimed so that no later poller (cycle, interact,
// examine, the Home/End synthesiser) acts on it too.
bool Took(acc::hotkeys::Action a) {
    if (!acc::hotkeys::Pressed(a)) return false;
    acc::hotkeys::Consume(a);
    return true;
}

}  // namespace

bool IsMenuOpen() { return g_state.open; }

void PollWin32() {
    using A = acc::hotkeys::Action;

    if (acc::hotkeys::Pressed(A::HelpContext)) SpeakContext();

    if (Took(A::HelpMenuOpen)) {
        if (g_state.open) Close(/*announce=*/true);
        else              Open();
    }
    if (!g_state.open) return;

    if (Took(A::NavUp))   FocusRow(g_state.focus - 1);
    if (Took(A::NavDown)) FocusRow(g_state.focus + 1);
    if (Took(A::NavHome)) FocusRow(0);
    if (Took(A::NavEnd))  FocusRow(g_state.rowCount - 1);
    if (Took(A::InteractTarget)) SpeakRow(g_state.focus, /*interrupt=*/true);  // Enter
    if (Took(A::SubmenuEsc)) {
        Close(/*announce=*/true);
        acc::input::NoteOverlayEscClosed();
    }
}

void Tick() {
    // A list opened in the world must not hold its pause across an area load.
    // Silent: the transition has its own announcements.
    if (!g_state.open || !g_state.pausedWorld) return;
    Vector p;
    if (acc::engine::GetPlayerPosition(p)) return;
    acclog::Write("Help", "self-disarm: player position lost");
    Close(/*announce=*/false);
}

}  // namespace acc::help
