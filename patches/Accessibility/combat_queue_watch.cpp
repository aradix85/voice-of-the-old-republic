// The live queue announce: attributing an engine-added action to the user's
// press, and saying when a press queued nothing.

#include "combat_queue_internal.h"

#include "engine_player.h"
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
    int preCount = RawRoundCount(combatRound);
    if (preCount < 0) return;

    char label[96] = "";
    BuildActionLabel(action, label, sizeof(label));
    if (!label[0]) std::strncpy(label, "?", sizeof(label) - 1);

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
