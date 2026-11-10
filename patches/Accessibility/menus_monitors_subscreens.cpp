// The in-game sub-screens (equipment, inventory, map, journal and the rest
// of the icon strip): their table, announcing a newly opened one, and arming
// the drill into it.

#include "menus_monitors_internal.h"

#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus.h"
#include "menus_journal.h"
#include "speech.h"

namespace acc::menus::monitors {

namespace {

constexpr uint32_t kNoStrref = 0xFFFFFFFFu;

// The quest-items modal's LBL_TITLE.
constexpr size_t kQuestItemsTitleLabelOffset = 0x768;

// strrefK1/strrefK2: KOTOR 2's dialog.tlk assigns KOTOR 1's ids to unrelated
// strings (48218 there is "Ablativplatten Mk 1"), so each game needs its own
// cluster — K1 48218..48225, K2 48620..48628. Same values as the icon table in
// menus_extract_icons.cpp (slot 4 there matches the PartySelection row on
// KOTOR 2 and the InGameMessages row on KOTOR 1); keep the two in sync.
// kotor2Only: the row is a sub-screen on KOTOR 2 alone; FindSpec skips it on
// KOTOR 1.
struct InGameSubScreenSpec {
    PanelKind   kind;
    uint32_t    strrefK1;
    uint32_t    strrefK2;
    const char* literal;     // when the game has no strref for it
    bool        kotor2Only;
};

const InGameSubScreenSpec k_inGameSubScreens[] = {
    { PanelKind::InGameEquip,     kNoStrref, 48620u, "Equipment" },
    { PanelKind::InGameInventory, 48220u,    48621u, "Inventory" },
    { PanelKind::InGameCharacter, 48225u,    48622u, "Character" },
    { PanelKind::InGameMap,       48221u,    48626u, "Map" },
    { PanelKind::InGameAbilities, 48224u,    48623u, "Abilities" },
    { PanelKind::InGameJournal,   48218u,    48627u, "Journal" },
    { PanelKind::InGameOptions,   48222u,    48628u, "Options" },
    // K2's own messages entry (48624) is empty in its TLK; 1563 carries the
    // caption there. KOTOR 2 opens the message log from the journal's
    // Message Log button (no strip icon).
    { PanelKind::InGameMessages,  48223u,    1563u,  "Messages" },
    // KOTOR 2 files the party screen in the strip's fifth slot. Its
    // SwitchToSWInGameGui (0x007CA550) opens the panel at [gui+0xc+id*4], and
    // id 4 lands on [gui+0x1c], the CSWGuiPartySelection slot
    // (engine_panels.cpp). So it opens like any strip sub-screen, the strip
    // stays in front, and only the drill routes the arrows into it; without
    // this row they stayed on the strip. 48625 is K2's caption for that slot.
    // On KOTOR 1 the party screen is a standalone panel and must not drill.
    { PanelKind::PartySelection,  kNoStrref, 48625u, "Party Selection",
      /*kotor2Only=*/true },
    // Quest-items modal pushed by the journal's "Quest Items" button. Not a
    // strip sub-screen; its title is read live from LBL_TITLE in the
    // announce path.
    { PanelKind::InGameQuestItems, kNoStrref, kNoStrref, nullptr },
};

void* s_visibleSubScreens[16];
int   s_visibleSubScreenCount = 0;

const InGameSubScreenSpec* FindSpec(PanelKind k) {
    for (const auto& s : k_inGameSubScreens) {
        if (s.kotor2Only && !acc::game::IsKotor2()) continue;
        if (s.kind == k) return &s;
    }
    return nullptr;
}

bool IsSubScreenTracked(void* p) {
    for (int i = 0; i < s_visibleSubScreenCount; ++i) {
        if (s_visibleSubScreens[i] == p) return true;
    }
    return false;
}

// Speaks a newly opened sub-screen's name: the quest-items modal's live
// title, otherwise the game's strref, otherwise the row's literal.
void AnnounceSubScreen(void* p, PanelKind k, const InGameSubScreenSpec& spec) {
    if (k == PanelKind::InGameQuestItems) {
        char title[128];
        void* titleLabel = reinterpret_cast<unsigned char*>(p) + kQuestItemsTitleLabelOffset;
        if (ReadGuiString(titleLabel, kLabelGuiStringPtrOffset, title, sizeof(title)) &&
            title[0] != '\0') {
            acclog::Write("Menus.SubScreen", "panel=%p kind=%s title=\"%s\" (dynamic)",
                          p, PanelKindName(k), title);
            acc::speech::Speak(title, /*interrupt=*/false);
        } else {
            acclog::Write("Menus.SubScreen", "panel=%p kind=%s LBL_TITLE empty; silent",
                          p, PanelKindName(k));
        }
        return;
    }
    char text[128];
    const uint32_t strref = acc::game::IsKotor2() ? spec.strrefK2 : spec.strrefK1;
    if (strref != kNoStrref && LookupTlk(strref, text, sizeof(text))) {
        acclog::Write("Menus.SubScreen", "panel=%p kind=%s strref=%u text=\"%s\"",
                      p, PanelKindName(k), strref, text);
        acc::speech::Speak(text, /*interrupt=*/false);
        return;
    }
    acclog::Write("Menus.SubScreen", "panel=%p kind=%s text=\"%s\" (literal)",
                  p, PanelKindName(k), spec.literal);
    acc::speech::Speak(spec.literal, /*interrupt=*/false);
}

}  // namespace

void AnnounceNewSubScreens(void** panels, int count) {
    void* nowVisible[16] = {};
    int   nowCount = 0;
    bool  newSubScreenDetected = false;
    for (int i = 0; i < count && nowCount < 16; ++i) {
        void* p = panels[i];
        if (!p) continue;
        PanelKind k = IdentifyPanel(p);
        const InGameSubScreenSpec* spec = FindSpec(k);
        if (!spec) continue;
        nowVisible[nowCount++] = p;
        if (IsSubScreenTracked(p)) continue;
        newSubScreenDetected = true;
        AnnounceSubScreen(p, k, *spec);
        // Diagnostic: confirm the journal surfaces every entry the engine has.
        if (k == PanelKind::InGameJournal) {
            acc::menus::journal::LogEntryCounts(p);
        }
    }
    memcpy(s_visibleSubScreens, nowVisible, sizeof(nowVisible));
    s_visibleSubScreenCount = nowCount;

    // Sole drill-arm site. Triggers on any newly-visible sub-screen kind
    // (Inventory, Map, Journal, …) regardless of which engine function
    // opened it — ShowSWInGameGui (cold path, e.g. Esc → Options),
    // SwitchToSWInGameGui (warm path, strip-icon Enter / hotkey while a
    // sub-screen is already up), our own Tab cycle queue, or any future
    // engine path. Polling panels[] is the only single-site approach that
    // covers them all (a hook on SwitchToSWInGameGui alone misses the cold
    // path and leaves the strip chain-navigable on the first Esc). One-tick
    // latency between push and arm is invisible to the user. Idempotent —
    // Tab cycling re-detects the same sub-screen the next tick but the guard
    // skips.
    if (newSubScreenDetected && !acc::menus::IsDrilledIntoSubScreen()) {
        acc::menus::SetDrilledIntoSubScreen(true);
        acclog::Write("Drill", "auto-armed on new sub-screen in panels[]");
    }
}

void* FindActiveSubScreenPanel() {
    constexpr int kCap = 32;
    void* panelData[kCap];
    int panelCount = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelData, kCap);
    // The journal stays in panels[] underneath the screens it opens (KOTOR 2's
    // message log, the quest-items list), and panels[] order says nothing
    // about which is on top — a closed tutorial popup can reshuffle it so the
    // journal comes first and takes the keys back. So the journal only wins
    // when nothing else is open.
    void* journal = nullptr;
    for (int i = 0; i < panelCount; ++i) {
        void* p = panelData[i];
        if (!p) continue;
        PanelKind k = IdentifyPanel(p);
        if (!FindSpec(k)) continue;
        if (k != PanelKind::InGameJournal) return p;
        if (!journal) journal = p;
    }
    return journal;
}

bool IsInGameSubScreenKind(PanelKind k) {
    return FindSpec(k) != nullptr;
}

}  // namespace acc::menus::monitors
