// The in-game sub-screens (equipment, inventory, map, journal and the rest of the icon strip): their table, announcing a newly opened one, and arming the drill into it.

#include "menus_monitors_internal.h"

#include "engine_game.h"      // IsKotor2 — per-game sub-screen title strrefs
#include "engine_offsets.h"
#include "engine_reads.h"
#include "menus.h"            // SetDrilledIntoSubScreen — sole drill-arm site
#include "menus_journal.h"   // LogEntryCounts — entry-count completeness diagnostic
#include "strings.h"
#include "speech.h"

namespace acc::menus::monitors {

namespace {

// ============================================================================
// Sub-screen tracking. Spec table + visible-set + announce-new pass. Used
// by MonitorPanelContents (this TU) and the drill router in menus.cpp
// (exposed via FindActiveSubScreenPanel + IsInGameSubScreenKind below).
// ============================================================================

const InGameSubScreenSpec k_inGameSubScreens[] = {
    { PanelKind::InGameEquip,     0, 0xFFFFFFFFu, 48620u, "Equipment" },
    { PanelKind::InGameInventory, 1, 48220u,      48621u, "Inventory" },
    { PanelKind::InGameCharacter, 2, 48225u,      48622u, "Character" },
    { PanelKind::InGameMap,       6, 48221u,      48626u, "Map" },
    { PanelKind::InGameAbilities, 3, 48224u,      48623u, "Abilities" },
    { PanelKind::InGameJournal,   5, 48218u,      48627u, "Journal" },
    { PanelKind::InGameOptions,   7, 48222u,      48628u, "Options" },
    // K2's own messages entry (48624) is empty in its TLK; 1563 carries the
    // caption there. guiId 4 is KOTOR 1's: KOTOR 2 opens the message log
    // from the journal's Message Log button instead (no strip icon).
    { PanelKind::InGameMessages,  4, 48223u,      1563u,  "Messages" },
    // KOTOR 2 files the party screen in the strip's fifth slot. Its
    // SwitchToSWInGameGui (0x007CA550) opens the panel at [gui+0xc+id*4], and
    // id 4 lands on [gui+0x1c], the CSWGuiPartySelection slot
    // (engine_panels.cpp). So it opens like any strip sub-screen, the strip
    // stays in front, and only the drill below routes the arrows into it.
    // Without this row they stayed on the strip (field log 2026-10-29,
    // patch-20261029-192616.log). 48625 is K2's caption for that slot. On
    // KOTOR 1 the party screen is a standalone panel and must not drill.
    { PanelKind::PartySelection,  4, 0xFFFFFFFFu, 48625u, "Party Selection",
      /*kotor2Only=*/true },
    // Quest-items modal pushed by the journal's "Quest Items" button.
    // Not a strip sub-screen (no icon → guiId -1); its title is read live from
    // LBL_TITLE in the announce path, so strref/literal are unused sentinels.
    { PanelKind::InGameQuestItems, -1, 0xFFFFFFFFu, 0xFFFFFFFFu, nullptr },
};

void* s_visibleSubScreens[16];
int   s_visibleSubScreenCount = 0;

// Per-game strref for a spec row. 0xFFFFFFFF means "no TLK entry" and the
// caller falls back to the literal / localised string table.
uint32_t SpecStrref(const InGameSubScreenSpec* s) {
    if (!s) return 0xFFFFFFFFu;
    return acc::game::IsKotor2() ? s->strrefK2 : s->strrefK1;
}

bool IsSubScreenTracked(void* p) {
    for (int i = 0; i < s_visibleSubScreenCount; ++i) {
        if (s_visibleSubScreens[i] == p) return true;
    }
    return false;
}

}  // namespace

const InGameSubScreenSpec* FindSpec(PanelKind k) {
    for (const auto& s : k_inGameSubScreens) {
        if (s.kotor2Only && !acc::game::IsKotor2()) continue;
        if (s.kind == k) return &s;
    }
    return nullptr;
}

void AnnounceNewSubScreens(void** panels, int count) {
    void* nowVisible[16];
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

        // QuestItem modal has no static strip strref/literal; read its
        // LBL_TITLE (+0x768) live so the announce is correct and localized.
        // Tracked via the normal s_visibleSubScreens machinery, so closing
        // and reopening re-announces.
        if (k == PanelKind::InGameQuestItems) {
            char title[128];
            void* titleLabel = reinterpret_cast<unsigned char*>(p) + 0x768;
            if (ReadGuiString(titleLabel, kLabelGuiStringPtrOffset,
                              title, sizeof(title)) && title[0] != '\0') {
                acclog::Write("Menus.SubScreen",
                              "panel=%p kind=%s title=\"%s\" (dynamic)",
                              p, PanelKindName(k), title);
                acc::speech::Speak(title, /*interrupt=*/false);
            } else {
                acclog::Write("Menus.SubScreen",
                              "panel=%p kind=%s LBL_TITLE empty; silent",
                              p, PanelKindName(k));
            }
            continue;
        }

        char text[128];
        bool spoke = false;
        const uint32_t strref = SpecStrref(spec);
        if (strref != 0xFFFFFFFFu &&
            LookupTlk(strref, text, sizeof(text))) {
            acclog::Write("Menus.SubScreen", "panel=%p kind=%s strref=%u text=\"%s\"",
                          p, PanelKindName(k), strref, text);
            acc::speech::Speak(text, /*interrupt=*/false);
            spoke = true;
        }
        if (!spoke) {
            // No-strref sub-screens (Equipment: the engine never asks for
            // its caption via TLK) can't reach the locale-stable strref
            // path, so route them through the localised string table rather
            // than the hardcoded German literal. The table literal stays as
            // an ASCII last-ditch fallback.
            const char* literal = spec->literal;
            if (strref == 0xFFFFFFFFu && k == PanelKind::InGameEquip) {
                literal = acc::strings::Get(acc::strings::Id::EquipMenuName);
            }
            acclog::Write("Menus.SubScreen", "panel=%p kind=%s text=\"%s\" (literal)",
                          p, PanelKindName(k), literal);
            acc::speech::Speak(literal, /*interrupt=*/false);
        }

        // (the character sheet no longer reads the full stat block on open —
        // legacy workaround from when the panel wasn't keyboard-navigable.
        // Stats are now reachable via in-panel arrow navigation; the panel
        // name spoken above is the sole open-announce, matching every
        // other strip sub-screen.)

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
    // covers them all (we tried hooking SwitchToSWInGameGui as the single
    // event source — it missed the cold path entirely and left the strip
    // chain-navigable on first Esc). One-tick latency between push and
    // arm is invisible to the user. Idempotent — Tab cycling re-detects
    // the same sub-screen the next tick but the guard skips.
    if (newSubScreenDetected && !acc::menus::IsDrilledIntoSubScreen()) {
        acc::menus::SetDrilledIntoSubScreen(true);
        acclog::Write("Drill",
                      "auto-armed on new sub-screen in panels[]");
    }
}

void* FindActiveSubScreenPanel() {
    constexpr int kCap = 32;
    void* panelData[kCap];
    int panelCount = acc::engine::ReadPanelArray(
        acc::engine::GetGuiManager(), panelData, kCap);
    // The journal stays in panels[] underneath the screens it opens (KOTOR 2's
    // message log, the quest-items list), and panels[] order says nothing
    // about which is on top — a closed tutorial popup reshuffled it so the
    // journal came first and took the keys back (patch-20260917-173602.log).
    // So the journal only wins when nothing else is open.
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
