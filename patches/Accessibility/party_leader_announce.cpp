#include "party_leader_announce.h"

#include <windows.h>
#include <cstring>

#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_player.h"
#include "hotkeys.h"
#include "log.h"
#include "speech.h"
#include "transitions.h"

namespace acc::party_leader_announce {

namespace {

// The engine handles Tab after our tick and swaps the leader some frames
// later. A Tab that swaps nobody (alone, or the cycle is blocked) answers
// with the current name once this much time has passed without a swap.
constexpr DWORD kTabWindowMs = 750;
bool  g_tabArmed = false;
DWORD g_tabTick  = 0;

// A load or a module change rebuilds the party: the leader "change" that
// follows is not the player's doing and would talk over the area name. The
// area pointer is the anchor; the load-from-save flag drops a moment before
// the new leader is readable.
constexpr DWORD kAreaChangeGraceMs = 3000;
void* g_lastArea       = nullptr;
DWORD g_areaChangeTick = 0;

char g_lastLeader[64] = "";

}  // namespace

void Tick() {
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::PartyLeaderAnnounce)) {
        g_tabArmed = true;
        g_tabTick  = GetTickCount();
    }

    // Polling the leader's name while the player slot is half built wedges
    // the load from character creation into the world, and during a module
    // handoff it reads into the old module's teardown.
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused) ||
        acc::transitions::IsModuleLoadPending()) {
        g_tabArmed = false;
        return;
    }

    void* area = acc::engine::GetCurrentArea();
    if (area && area != g_lastArea) {
        g_lastArea       = area;
        g_areaChangeTick = GetTickCount();
    }

    char now[64] = "";
    if (!acc::engine::GetActiveLeaderName(now, sizeof(now)) || now[0] == '\0') return;

    if (std::strcmp(now, g_lastLeader) == 0) {
        if (g_tabArmed && GetTickCount() - g_tabTick >= kTabWindowMs) {
            g_tabArmed = false;
            acclog::Write("PartyLeader", "Tab swapped nobody; speaking [%s]", now);
            acc::speech::Speak(now, /*interrupt=*/true);
        }
        return;
    }

    const bool first = g_lastLeader[0] == '\0';
    const bool byTab = g_tabArmed;
    const bool partyRebuilt = g_areaChangeTick != 0 &&
                              GetTickCount() - g_areaChangeTick < kAreaChangeGraceMs;
    strncpy_s(g_lastLeader, now, _TRUNCATE);
    g_tabArmed = false;

    if (first || (partyRebuilt && !byTab)) {
        acclog::Write("PartyLeader", "leader is [%s]; not spoken (%s)", now,
                      first ? "first seen" : "the party was just rebuilt");
        return;
    }
    acclog::Write("PartyLeader", "leader changed to [%s]%s", now, byTab ? " by Tab" : "");
    acc::speech::Speak(now, /*interrupt=*/true);
}

}  // namespace acc::party_leader_announce
