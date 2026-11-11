// Combat mode: "Combat begins", "Combat ends" and the leader at peace while
// the party still fights.

#include "combat.h"

#include <windows.h>

#include "engine_app.h"           // GetClientApp
#include "engine_offsets.h"
#include "engine_player.h"        // IsAnyPartyMemberInCombat
#include "log.h"
#include "speech.h"
#include "stable_flag.h"
#include "strings.h"
#include "transitions.h"          // IsModuleLoadPending
#include "unified_action_menu.h"  // closed when the encounter ends

namespace acc::combat {

namespace {

// CClientExoApp::GetCombatMode: 0 = peace. The flag lives on the internal
// struct; the accessor on the facade walks to it.
typedef int (__thiscall* PFN_GetCombatMode)(void* this_);

bool ReadCombatMode(int& outMode) {
    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) return false;
    __try {
        auto fn = reinterpret_cast<PFN_GetCombatMode>(kAddrGetCombatMode);
        outMode = fn(exoApp);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

constexpr uint32_t kCombatModeQuietMs = 200;  // collapses oscillation at the edges

void Announce(acc::strings::Id id) {
    const char* phrase = acc::strings::Get(id);
    acc::speech::Speak(phrase, /*interrupt=*/false);
    acclog::Write("Combat.Mode", "-> [%s]", phrase);
}

}  // namespace

bool IsCombatActive() {
    int mode = 0;
    return ReadCombatMode(mode) && mode != 0;
}

bool IsPartyInCombat() {
    return acc::engine::IsAnyPartyMemberInCombat() || IsCombatActive();
}

void TickCombatMode() {
    // The accessor must not run through a module load: probing it during a
    // stunt-module cutscene load tripped the engine's LYT loader on the
    // previous module's freed resref arena.
    if (acc::transitions::IsModuleLoadPending()) return;

    int mode = 0;
    if (!ReadCombatMode(mode)) return;

    // Two signals. The engine global (GetCombatMode) mirrors the controlled
    // leader's combat bit only and is re-synced on every Tab
    // (CSWParty::SetLeader -> SetCombatMode), so it reads peace after a
    // switch to a member who is not engaged yet. The encounter is the OR of
    // every party member's own bit; begin, end and the menu close follow
    // that one, so that a party switch cannot fake a combat end.
    const bool leaderInCombat = mode != 0;
    const bool partyInCombat =
        acc::engine::IsAnyPartyMemberInCombat() || leaderInCombat;
    const DWORD now = GetTickCount();

    static acc::StableFlag s_party(kCombatModeQuietMs);
    static acc::StableFlag s_leader(kCombatModeQuietMs);

    if (s_party.Update(partyInCombat, now)) {
        const bool began = s_party.Settled();
        Announce(began ? acc::strings::Id::CombatBegins : acc::strings::Id::CombatEnds);
        // The action menu is a paused queueing surface: left open across the
        // end of a fight, the first Enter afterwards lands on a menu entry
        // and not on the world object the player meant.
        if (!began && acc::unified_menu::IsActive()) {
            acc::unified_menu::ForceDisarm("combat-end");
        }
    }

    // The leader at peace while the encounter goes on: a Tab to a member who
    // is not engaged, or the leader breaking off. On a real end both signals
    // fall together and "Combat ends" speaks. The other direction, a Tab onto
    // a fighting member, is covered by the leader-name announcement.
    if (s_leader.Update(leaderInCombat, now) && !s_leader.Settled() && partyInCombat) {
        Announce(acc::strings::Id::CombatLeaderAtPeace);
    }
}

}  // namespace acc::combat
