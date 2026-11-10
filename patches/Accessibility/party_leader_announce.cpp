#include "party_leader_announce.h"

#include <windows.h>
#include <cstring>

#include "engine_area.h"      // GetCurrentArea
#include "engine_offsets.h"   // Vector (global struct)
#include "engine_player.h"    // GetPlayerPosition, GetClientLeader,
                              //   GetActiveLeaderName
#include "hotkeys.h"
#include "log.h"
#include "speech.h"
#include "transitions.h"       // IsModuleLoadPending

namespace acc::party_leader_announce {

namespace {

// Tab arms a pending announce; the actual read happens on a *later* tick,
// once the engine has had time to process the keypress and swap the
// active leader. Our hotkey rising-edge detector runs from
// core_tick::Dispatch, which the engine drives before its own per-frame
// input pump processes Tab and re-wires CClientExoApp::GetPlayerCreature
// to the next party member. Reading on the same tick gets the *old*
// leader — confirmed live by the user 2026-05-23 ("Tab announces the
// previous leader, shifted by one").
//
// Two pieces of state:
//   g_pendingTicks  — countdown after Tab press. >0 means "keep watching
//                     for the leader swap". 0 means idle.
//   g_pendingLeader — the leader pointer captured at Tab press time.
//                     We announce when GetClientLeader() returns a
//                     *different* pointer, or when the countdown
//                     expires (the solo-party case: leader pointer
//                     never changes; speak the current name as the
//                     "still solo" UX signal the header doc calls out).
//
// 6 ticks ≈ ~100 ms at 60 fps — plenty of slack for the engine to swap,
// short enough that the announce stays tightly coupled to the keypress.
constexpr int kPendingWindowTicks = 6;
int   g_pendingTicks  = 0;
void* g_pendingLeader = nullptr;

void Speak() {
    char creatureName[64] = {0};
    if (!acc::engine::GetActiveLeaderName(creatureName, sizeof(creatureName))) {
        acclog::Write("PartyLeader", "Tab — no leader name resolved");
        return;
    }
    acclog::Write("PartyLeader", "Tab — speaking leader=[%s]", creatureName);
    acc::speech::Speak(creatureName, /*interrupt=*/true);
}

}  // namespace

void Tick() {
    // Tab rising edge + foreground gate via the central registry. Tab is
    // the universal alt-tab modifier outside KOTOR, so the foreground
    // gate matters; Pressed() covers it.
    if (acc::hotkeys::Pressed(acc::hotkeys::Action::PartyLeaderAnnounce)) {
        // Player-loaded gate — title screen / module-load have no party.
        Vector unused;
        if (!acc::engine::GetPlayerPosition(unused)) return;

        // Arm: capture pre-press leader, start watching for the engine to
        // swap it. Each new Tab press extends the window — a rapid double-
        // tap will announce the most recent leader rather than two stale
        // names.
        g_pendingLeader = acc::engine::GetClientLeader();
        g_pendingTicks  = kPendingWindowTicks;
        acclog::Write("PartyLeader",
                      "Tab armed — pre-press leader=%p, window=%d ticks",
                      g_pendingLeader, g_pendingTicks);
        return;
    }

    if (g_pendingTicks <= 0) return;

    void* current = acc::engine::GetClientLeader();
    if (current && current != g_pendingLeader) {
        // Engine swapped leader. Announce the new one.
        acclog::Write("PartyLeader",
                      "Tab — leader swapped (%p → %p), speaking",
                      g_pendingLeader, current);
        g_pendingTicks  = 0;
        g_pendingLeader = nullptr;
        Speak();
        return;
    }

    if (--g_pendingTicks == 0) {
        // Window expired with no swap. Solo-party / cycle-blocked case —
        // speak the current leader anyway so the user gets confirmation
        // that Tab did something (or that they're still solo).
        acclog::Write("PartyLeader",
                      "Tab — window expired with no swap (leader=%p), "
                      "speaking current",
                      current);
        g_pendingLeader = nullptr;
        Speak();
    }
}

void TickLeaderChange() {
    // Player-loaded gate — don't probe CClientExoApp::GetPlayerCharacterName
    // until the world has actually loaded. During the chargen→world
    // transient, the player slot is half-initialised and hammering the
    // engine accessor every frame intermittently wedges the load (the
    // world never spins up, engine alive but no module ever loads).
    // Reproduced 2026-05-19 on chargen→world: Combat.PcStat fires once
    // with "temp" at frame ~667, then 1300+ frames tick with no further
    // engine state change until the user gives up and quits.
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    // Module-load latch — covers the cutscene-load transient that
    // GetPlayerPosition can't catch (old module still alive, gate keeps
    // returning true straight through the handoff). PartySelection-OK →
    // stunt_03a crashed deep in CLYT::LoadLayout on a decommitted
    // resref page in dump swkotor.exe(1).23224.dmp; the spammy
    // `PartyLeader: leader=handle —` log from this probe was the only
    // per-tick activity firing into the engine's teardown window.
    if (acc::transitions::IsModuleLoadPending()) return;

    static char  s_lastLeader[64]   = "";
    static void* s_lastArea         = nullptr;
    static DWORD s_areaChangeTick    = 0;

    // Track area-pointer changes. A save-game load OR a module transition
    // re-establishes the party, so the leader "change" that follows is the
    // engine wiring up the new party — not a user Tab-switch — and announcing
    // the PC's name talks over the area-name cue. We re-baseline silently for
    // a short grace window after any area change. The load_from_savegame flag
    // can't anchor this: it clears a hair before the new leader becomes
    // readable (patch-20260614-165512.log — "Area: …" and the leader
    // change both land after the flag has dropped), so the gate missed. The
    // area-pointer change is the robust anchor — it's exactly what
    // transitions::SpeakArea fires on.
    void* area = acc::engine::GetCurrentArea();
    if (area && area != s_lastArea) {
        s_lastArea       = area;
        s_areaChangeTick = GetTickCount();
    }

    char now[64] = "";
    if (!acc::engine::GetActiveLeaderName(now, sizeof(now))) return;
    if (now[0] == '\0') return;
    if (std::strcmp(now, s_lastLeader) == 0) return;

    bool wasFirstObservation = (s_lastLeader[0] == '\0');
    // ~3s covers the lag between the area pointer surfacing and the new
    // leader name becoming readable (observed simultaneous, same second).
    constexpr DWORD kAreaChangeGraceMs = 3000;
    bool recentAreaChange =
        s_areaChangeTick != 0 &&
        (GetTickCount() - s_areaChangeTick) < kAreaChangeGraceMs;
    std::strncpy(s_lastLeader, now, sizeof(s_lastLeader) - 1);
    s_lastLeader[sizeof(s_lastLeader) - 1] = '\0';

    if (wasFirstObservation) {
        acclog::Write("Combat.PcStat", "first leader observed=[%s]; suppress",
                      now);
        return;
    }
    if (recentAreaChange) {
        acclog::Write("Combat.PcStat",
                      "leader changed -> [%s] %lums after area change; suppress "
                      "(baseline updated)", now,
                      (unsigned long)(GetTickCount() - s_areaChangeTick));
        return;
    }
    acclog::Write("Combat.PcStat", "leader changed -> [%s]; speaking name only",
                  now);
    acc::speech::Speak(now, /*interrupt=*/true);
}

}  // namespace acc::party_leader_announce
