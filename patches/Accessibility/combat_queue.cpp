// The Shift+H combat queue overlay: every party member's queued actions as
// one list, with Enter to remove the focused one and Shift+Enter to clear
// all.

#include "combat_queue_internal.h"

#include "engine_area.h"
#include "engine_input.h"
#include "engine_player.h"
#include "engine_subscreen.h"
#include "hotkeys.h"
#include "log.h"
#include "same_name_suffix.h"
#include "speech.h"
#include "transitions.h"
#include "unified_action_menu.h"
#include "engine_rebase.h"

namespace acc::combat::queue {

namespace {

// CSWSCombatRound::RemoveLastAction: a single tail remove and free, 1 on
// success and 0 on an empty queue in both games. The engine's only per-round
// remove, so a row can only go when it is its creature's last.
typedef int  (__thiscall* PFN_RemoveLastAction)(void* combatRound);
const uintptr_t kAddrCombatRoundRemoveLastAction =
    acc::addr::Pick(0x004d37b0, 0x005904C0);

bool RemoveLastAction(void* combatRound) {
    __try {
        reinterpret_cast<PFN_RemoveLastAction>(kAddrCombatRoundRemoveLastAction)(
            combatRound);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// One row per queued action across the party, rebuilt on every open,
// keypress and tick: the engine drains queues as the combat round advances.
struct Row {
    void* creature;          // CSWSCreature*, the action's owner
    void* combatRound;       // CSWSCombatRound*
    int   perCreatureIdx;    // the row's place in its owner's queue
    int   perCreatureCount;  // its owner's queue length
    char  charName[64];
};

constexpr int kMaxRows = 32;  // 3 members x up to ~10 actions, with headroom

struct State {
    bool active   = false;
    int  focusIdx = 0;  // 0-based focus into rows[0..count)
    int  count    = 0;
    Row  rows[kMaxRows];
};

State g_state;

// A companion's name; the chargen name only guards an empty read.
void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return;
    outBuf[0] = '\0';
    if (acc::engine::GetObjectDisplayNameByHandle(handle, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return;
    }
    acc::engine::GetPlayerCharacterName(outBuf, bufSize);
}

void AddRows(void* creature, void* round, int local, const char* name) {
    for (int i = 0; i < local && g_state.count < kMaxRows; ++i) {
        Row& r = g_state.rows[g_state.count++];
        r.creature = creature;
        r.combatRound = round;
        r.perCreatureIdx = i;
        r.perCreatureCount = local;
        strncpy_s(r.charName, name, _TRUNCATE);
    }
}

// Rebuild g_state.rows from the engine. Returns the row count.
int BuildRows() {
    g_state.count = 0;

    // The controlled creature first: the party table holds NPC roster slots
    // (companions), and the PC is no roster NPC.
    void* leader = acc::engine::GetPlayerServerCreature();
    if (void* round = ReadCombatRound(leader)) {
        char name[64] = "";
        acc::engine::GetActiveLeaderName(name, sizeof(name));
        AddRows(leader, round, CountQueueEntries(round), name);
    }

    // Then the companions, skipping the controlled one (Tab can hand control
    // to a companion).
    uint32_t handles[kPartyTableMaxMembers] = {};
    int      n = acc::engine::GetPartyMembers(
        handles, kPartyTableMaxMembers);

    for (int m = 0; m < n; ++m) {
        uint32_t handle = handles[m];
        if (handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId) {
            continue;
        }
        void* creature = acc::engine::ResolveServerObjectHandle(handle);
        if (!creature) {
            // Some handles come in the client namespace; the client
            // resolver returns the server object directly.
            creature = acc::engine::ResolveClientObjectHandle(handle);
        }
        if (!creature || creature == leader) continue;
        void* round = ReadCombatRound(creature);
        if (!round) continue;
        int local = CountQueueEntries(round);
        if (local <= 0) continue;

        char name[64] = "";
        ResolveMemberName(handle, name, sizeof(name));
        AddRows(creature, round, local, name);
    }
    return g_state.count;
}

// Speak the focused row at index `idx` (0-based) of `count` total.
void SpeakRow(int idx) {
    if (idx < 0 || idx >= g_state.count) return;
    const Row& row = g_state.rows[idx];
    void* action = GetQueueAction(row.combatRound, row.perCreatureIdx);

    unsigned char type = 0xff;
    uint32_t target = 0;
    ReadActionFields(action, type, target);

    char verbBuf[96] = "";
    BuildActionLabel(action, verbBuf, sizeof(verbBuf));
    const char* verb = verbBuf;

    char tgtName[64] = "";
    if (target != 0u && target != kInvalidObjectId) {
        void* tgtObj = acc::engine::ResolveServerObjectHandle(target);
        if ((!acc::engine::GetObjectDisplayNameByHandle(
                 target, tgtName, sizeof(tgtName)) ||
             tgtName[0] == '\0') &&
            tgtObj) {
            acc::engine::GetObjectName(tgtObj, tgtName, sizeof(tgtName));
        }
        if (tgtName[0] != '\0' && tgtObj) {
            acc::narration::AppendSuffix(tgtObj, tgtName, sizeof(tgtName));
        }
    }

    char msg[256];
    std::snprintf(msg, sizeof(msg),
                  acc::strings::Get(acc::strings::Id::FmtQueueRow),
                  row.charName, verb, tgtName, idx + 1, g_state.count);
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Combat.Queue",
                  "row %d/%d char=[%s] type=%u target=0x%08x verb=[%s] "
                  "tgt=[%s]",
                  idx + 1, g_state.count, row.charName, (unsigned)type,
                  target, verb, tgtName);
}

// Removes row `idx` if it is the last of its creature's queue.
bool RemoveRow(int idx) {
    if (idx < 0 || idx >= g_state.count) return false;
    const Row& row = g_state.rows[idx];
    if (row.perCreatureIdx != row.perCreatureCount - 1) {
        acclog::Write("Combat.Queue",
                      "remove flat=%d char=[%s] local=%d/%d -> non-tail "
                      "remove not implemented",
                      idx, row.charName, row.perCreatureIdx,
                      row.perCreatureCount);
        return false;
    }
    return RemoveLastAction(row.combatRound);
}

// Back to front, so every remove takes its creature's last action.
bool ClearAllRows() {
    int total = g_state.count;
    int removed = 0;
    for (int i = total - 1; i >= 0; --i) {
        if (!RemoveLastAction(g_state.rows[i].combatRound)) break;
        ++removed;
    }
    acclog::Write("Combat.Queue", "ClearAllRows removed=%d/%d",
                  removed, total);
    return removed > 0;
}

}  // namespace

bool IsActive() { return g_state.active; }

void ForceDisarm(const char* reason) {
    if (!g_state.active) return;
    acclog::Write("Combat.Queue", "disarm reason=%s",
                  reason ? reason : "?");
    g_state.active = false;
    acc::engine::EndOverlayPause(acc::engine::OverlayPauseOwner::CombatQueue);
    g_state.focusIdx = 0;
    g_state.count = 0;
}

bool Open() {
    int count = BuildRows();
    if (count <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::QueueEmpty),
                    /*interrupt=*/true);
        acclog::Write("Combat.Queue", "Open — party queue empty; not arming");
        return false;
    }

    g_state.active   = true;
    acc::engine::BeginOverlayPause(acc::engine::OverlayPauseOwner::CombatQueue);
    g_state.focusIdx = 0;

    char msg[128];
    std::snprintf(msg, sizeof(msg),
                  acc::strings::Get(acc::strings::Id::FmtQueueOpen),
                  count);
    acc::speech::Speak(msg, /*interrupt=*/true);
    acclog::Write("Combat.Queue", "ARMED rows=%d -> [%s]", count, msg);

    SpeakRow(0);
    return true;
}

void ClearAllAndClose() {
    if (!g_state.active) return;
    // Re-read first: the engine drains the queue as the combat round advances.
    if (BuildRows() <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::QueueEmpty),
                     /*interrupt=*/true);
        ForceDisarm("queue-emptied");
        return;
    }
    const bool ok = ClearAllRows();
    acc::speech::Speak(acc::strings::Get(ok ? acc::strings::Id::QueueCleared
                                      : acc::strings::Id::QueueRemoveFailed),
                 /*interrupt=*/true);
    acclog::Write("Combat.Queue", "clear-all ok=%d", ok ? 1 : 0);
    ForceDisarm("clear-all");
}

bool HandleInputEvent(int code, int value) {
    if (!g_state.active) return false;
    if (value == 0) return false;  // press-edge only

    // Refresh on every keypress — the engine can drain the queue between
    // ticks (combat round advancing) and we don't want to dispatch a
    // remove against a stale row.
    int count = BuildRows();
    if (count <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::QueueEmpty),
                    /*interrupt=*/true);
        ForceDisarm("queue-emptied");
        return true;
    }
    if (g_state.focusIdx >= count) g_state.focusIdx = count - 1;
    if (g_state.focusIdx < 0)      g_state.focusIdx = 0;

    switch (code) {
        case kInputNavUp:
            if (g_state.focusIdx > 0) --g_state.focusIdx;
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputNavDown:
            if (g_state.focusIdx + 1 < count) ++g_state.focusIdx;
            SpeakRow(g_state.focusIdx);
            return true;
        case kInputEnter1:
        case kInputEnter2: {
            if (acc::hotkeys::ShiftHeld()) {
                ClearAllAndClose();
                return true;
            }

            // Capture the verb before the remove so the confirmation
            // phrase still has it.
            const Row& row = g_state.rows[g_state.focusIdx];
            void* action = GetQueueAction(row.combatRound,
                                          row.perCreatureIdx);
            unsigned char type = 0xff;
            uint32_t target = 0;
            ReadActionFields(action, type, target);
            const char* verb = acc::strings::Get(VerbForActionType(type));

            bool ok = RemoveRow(g_state.focusIdx);
            if (!ok) {
                acc::speech::Speak(acc::strings::Get(
                                acc::strings::Id::QueueRemoveFailed),
                            /*interrupt=*/true);
                return true;
            }
            char msg[128];
            std::snprintf(msg, sizeof(msg),
                          acc::strings::Get(
                              acc::strings::Id::FmtQueueRemoved),
                          verb);
            acc::speech::Speak(msg, /*interrupt=*/true);
            acclog::Write("Combat.Queue",
                          "Removed flat=%d type=%u verb=[%s]",
                          g_state.focusIdx + 1, (unsigned)type, verb);

            int newCount = BuildRows();
            if (newCount <= 0) {
                ForceDisarm("queue-emptied-after-remove");
                return true;
            }
            if (g_state.focusIdx >= newCount) {
                g_state.focusIdx = newCount - 1;
            }
            SpeakRow(g_state.focusIdx);
            return true;
        }
        case kInputEsc1:
        case kInputEsc2: {
            // Disarm first so EndOverlayPause clears our pause-owner bit. An
            // action menu still open underneath keeps holding the world
            // pause, so the user lands back in it, re-announced.
            ForceDisarm("esc");
            if (acc::unified_menu::IsActive()) {
                acclog::Write("Combat.Queue",
                              "Esc -> close; unified menu still active, "
                              "re-announcing (world stays paused)");
                acc::unified_menu::ReannounceCurrent();
            } else {
                acc::speech::Speak(acc::strings::Get(acc::strings::Id::QueueClosed),
                            /*interrupt=*/true);
                acclog::Write("Combat.Queue", "Esc -> close");
            }
            return true;
        }
        default:
            return false;
    }
}

void Tick() {
    if (!g_state.active) return;
    // The party's server creatures can't be trusted during a module load.
    if (acc::transitions::IsModuleLoadPending()) return;
    int count = BuildRows();
    if (count <= 0) {
        acc::speech::Speak(acc::strings::Get(acc::strings::Id::QueueEmpty),
                    /*interrupt=*/false);
        ForceDisarm("tick-queue-empty");
        return;
    }
}

void PollWin32Hotkey() {
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::CombatQueueOpen)) return;

    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    Open();
}

}  // namespace acc::combat::queue
