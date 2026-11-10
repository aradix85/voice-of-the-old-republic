// The live queue announce: attributing an engine-added action to the user's
// press, and saying when a press queued nothing.

#include "combat_queue_internal.h"

#include "engine_area.h"       // AreaObjectIterator, positions
#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_panels.h"     // HasActiveDialogPanel, IsForegroundUiBlocking
#include "engine_player.h"
#include "engine_reads.h"      // ReadCreatureCurrentHitPoints
#include "engine_subscreen.h"  // WorldIsPaused
#include "examine_view.h"      // IsHostileCreature
#include "filter_objects.h"
#include "log.h"
#include "speech.h"

namespace acc::combat::queue {

namespace {

// The GetTickCount() of the last user press, 0 when not armed. The engine
// auto-queues the leader's default attack through the same AddAction on the
// same round, so only this freshness window tells a press from an
// auto-attack.
DWORD g_userAddArmTick = 0;
// Window from press to the resulting AddAction. The dispatch is same-frame or
// next-tick (tens of ms); the leader's auto-attacks fire on the round cadence
// (~seconds) only when the queue is empty, so they fall outside. Refresh-on-
// arm + no-consume keeps a rapid press burst all announcing.
constexpr DWORD kUserAddWindowMs = 250;

// No-op watch (see CancelNoOpWatch in the header). Armed with the same press,
// cleared by an attributed add, and judged once the attribution window has
// closed — a little past it, so an add that only just made the window has been
// counted before we declare nothing happened.
DWORD g_noOpWatchTick = 0;
constexpr DWORD kNoOpVerdictMs = kUserAddWindowMs + 100;

int g_prePressDepth = -1;

// A living hostile creature within this many metres of the player.
constexpr float kIdleEnemyRangeM = 25.0f;
bool HostileNearby() {
    Vector p;
    if (!acc::engine::GetPlayerPosition(p)) return false;
    void* area = acc::engine::GetCurrentArea();
    if (!area) return false;
    acc::engine::AreaObjectIterator it(area);
    while (void* obj = it.Next()) {
        if (!acc::filter::ObjectMatches(obj, acc::filter::CycleCategory::Npc)) continue;
        if (!acc::examine_view::IsHostileCreature(obj)) continue;
        const int hp = acc::engine::ReadCreatureCurrentHitPoints(obj);
        if (hp == 0 || hp < -1) continue;
        Vector q;
        if (!acc::engine::GetObjectPosition(obj, q)) continue;
        if (DistXYSq(q, p) <= kIdleEnemyRangeM * kIdleEnemyRangeM) return true;
    }
    return false;
}

// Idle notice: in combat, not paused, nothing queued for the controlled
// character and an enemy still near, for this long -> one spoken line. An
// attack carries on by itself; this line says when R is actually needed.
constexpr DWORD kIdleNoticeMs = 2500;
DWORD g_idleSince  = 0;
bool  g_idleSpoken = false;

}  // namespace

void ArmUserQueueAdd() { g_userAddArmTick = GetTickCount(); }

void ArmNoOpWatch() { g_noOpWatchTick = GetTickCount(); }

void CancelNoOpWatch() { g_noOpWatchTick = 0; }

void TickNoOpWatch() {
    if (g_noOpWatchTick == 0) return;
    if ((GetTickCount() - g_noOpWatchTick) <= kNoOpVerdictMs) return;
    g_noOpWatchTick = 0;
    acclog::Write("Combat.Queue",
                  "user fire produced no queue add within %ums — announcing "
                  "that nothing happened", kNoOpVerdictMs);
    acc::speech::Speak(acc::strings::Get(acc::strings::Id::ActionRefused),
                 /*interrupt=*/false);
}

void OnEngineActionAdded(void* combatRound, void* action) {
    if (!combatRound) return;

    // Only the controlled creature's queue is the user's (it follows Tab).
    // The detour fires for every creature's round; the others are skipped
    // without a log line.
    void* leader = acc::engine::GetPlayerServerCreature();
    if (!leader) return;
    void* leaderRound = ReadCombatRound(leader);
    if (!leaderRound || combatRound != leaderRound) return;

    DWORD armed = g_userAddArmTick;
    if (armed == 0 || (GetTickCount() - armed) > kUserAddWindowMs) {
        acclog::Write("Combat.Queue",
                      "engine-add on leader round, no user arm — silent "
                      "(auto-action)");
        return;
    }

    g_noOpWatchTick = 0;  // the press did something; the cue below says what

    // Pre-add count (detour fires at function entry, before the node is
    // inserted). Engine cap is `if (3 < count)`: 0..3 accept → slot count+1,
    // >=4 reject → "queue full".
    int preCount = ReadRoundActionCount(combatRound);
    if (preCount < 0) return;

    char label[96] = "";
    BuildActionLabel(action, label, sizeof(label));
    if (!label[0]) strncpy_s(label, "?", _TRUNCATE);

    char msg[192];
    if (preCount >= 4) {
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(acc::strings::Id::FmtFireQueueFull),
                      label);
    } else {
        std::snprintf(msg, sizeof(msg),
                      acc::strings::Get(acc::strings::Id::FmtFireAtPosition),
                      label, preCount + 1);
    }
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Combat.Queue",
                  "engine-add slot=%d full=%d label=[%s] -> [%s]",
                  preCount + 1, preCount >= 4 ? 1 : 0, label, msg);
}

void TickIdleNotice() {
    if (acc::game::IsKotor2()) return;
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;
    void* server = acc::engine::GetPlayerServerCreature();
    const int qs = ReadQueueSize(server);
    const int oq = ReadOuterQueueSize(server);

    // Cheap checks first; the area walk only runs once the controlled
    // character has stood with an empty queue for a while.
    const bool idleNow =
        qs == 0 && oq == 0 &&
        acc::engine::IsAnyPartyMemberInCombat() &&
        !acc::engine::WorldIsPaused() &&
        !acc::engine::HasActiveDialogPanel() &&
        !acc::engine::IsForegroundUiBlocking();
    if (!idleNow) {
        g_idleSince = 0;
        if (qs > 0 || oq > 0 || !acc::engine::IsAnyPartyMemberInCombat()) {
            g_idleSpoken = false;
        }
        return;
    }
    const DWORD tick = GetTickCount();
    if (g_idleSince == 0) {
        g_idleSince = tick;
        return;
    }
    if (g_idleSpoken || tick - g_idleSince < kIdleNoticeMs) return;
    g_idleSpoken = true;
    if (!HostileNearby()) return;
    const char* line = acc::strings::Get(acc::strings::Id::CombatIdleNotice);
    acc::speech::Speak(line, /*interrupt=*/false);
    acclog::Write("Combat.Diag", "idle notice -> [%s]", line);
}

void ReportPrePressDepth() {
    g_prePressDepth = CountPlayerEntries();
}

int GetPrePressDepth() {
    // Consume on read: reset to the sentinel so a later press that did NOT go
    // through ReportPrePressDepth (e.g. a Shift+combo consumed in input_pipeline
    // before the bare-key prep, surfacing in the announce path only via the
    // shift-release race) reads -1 instead of this press's stale depth. The
    // announce treats -1 as "engine didn't dispatch a bare key" and stays silent.
    int d = g_prePressDepth;
    g_prePressDepth = -1;
    return d;
}

}  // namespace acc::combat::queue
