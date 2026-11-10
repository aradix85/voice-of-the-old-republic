
#include "combat_queue_internal.h"

#include "engine_area.h"      // ResolveServerObjectHandle, GetObjectName,
                              // GetObjectDisplayNameByHandle
#include "engine_input.h"
#include "engine_player.h"    // GetPlayerServerCreature, GetPartyMembers,
#include "engine_subscreen.h" // Begin/EndOverlayPause — freeze the world while open
                              // GetPlayerCharacterName
#include "hotkeys.h"
#include "log.h"
#include "same_name_suffix.h" // AppendSuffix for same-LocName disambiguator
#include "speech.h"
#include "transitions.h"      // IsModuleLoadPending — gate during cutscene-load
                              // transient (engine LYT loader use-after-free)
#include "unified_action_menu.h"  // IsActive / ReannounceCurrent — return to the
#include "engine_rebase.h"
                                  // menu underneath the queue without unpausing

namespace acc::combat::queue {

namespace {

// Engine surface — combat round + linked-list walker.
typedef int  (__thiscall* PFN_RemoveLastAction)(void* combatRound);

const uintptr_t kAddrCombatRoundRemoveLastAction =
    // K2 twin 0x005904C0: single tail-remove + free, returns 1 on success,
    // 0 when the queue is empty — K1's exact contract.
    acc::addr::Pick(0x004d37b0, 0x005904C0);  // CSWSCombatRound::RemoveLastAction

struct State {
    bool active   = false;
    int  focusIdx = 0;  // 0-based focus into rows[0..count)
    int  count    = 0;
    Row  rows[kMaxRows];
};

State g_state;

// Resolve the display name for a companion party member by handle.
// GetObjectDisplayNameByHandle returns the engine's universal localized
// name (Trask, Carth, ...). The controlled creature (PC) is named
// separately in BuildRows via GetActiveLeaderName, so this path only
// sees companions; the chargen-name fallback is kept purely as a guard
// for the rare empty-display-name read.
void ResolveMemberName(uint32_t handle, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return;
    outBuf[0] = '\0';
    if (acc::engine::GetObjectDisplayNameByHandle(handle, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return;
    }
    acc::engine::GetPlayerCharacterName(outBuf, bufSize);
}

// Rebuild g_state.rows from live engine state. Returns the row count.
// Preserves the focus if possible (clamped to the new size).
int BuildRows() {
    g_state.count = 0;

    // The controlled creature's queue comes FIRST, via GetPlayerServer
    // creature — the same accessor CountPlayerEntries (the "position N"
    // announce) uses. This is the only path that reliably reaches the
    // player's own combat round: the party-table walk below resolves NPC
    // *roster* slots (companions), and the PC is not a roster NPC, so it
    // never appears there. Sourcing it from the party walk produced an
    // empty queue while the player had actions queued (fixed 2026-06-07).
    void* leader = acc::engine::GetPlayerServerCreature();
    if (leader) {
        void* round = ReadCombatRound(leader);
        if (round) {
            int local = CountQueueEntries(round);
            for (int i = 0; i < local && g_state.count < kMaxRows; ++i) {
                Row& r = g_state.rows[g_state.count++];
                r.creature = leader;
                r.combatRound = round;
                r.perCreatureIdx = i;
                r.perCreatureCount = local;
                r.charName[0] = '\0';
                acc::engine::GetActiveLeaderName(r.charName,
                                                 sizeof(r.charName));
            }
        }
    }

    // Append every other party member's queue. GetPartyMembers resolves
    // pt_member_ids (NPC roster slots) to live companion handles. Skip any
    // companion that resolves to the controlled creature — Tab can hand
    // control to a companion, in which case it would appear both here and
    // as `leader` above.
    uint32_t handles[kPartyTableMaxMembers] = {};
    int      n = acc::engine::GetPartyMembers(
        handles, kPartyTableMaxMembers);

    for (int m = 0; m < n; ++m) {
        uint32_t handle = handles[m];
        if (handle == 0u || handle == 0xFFFFFFFFu ||
            handle == 0x7F000000u) {
            continue;
        }
        void* creature = acc::engine::ResolveServerObjectHandle(handle);
        if (!creature) {
            // Some handles come from the engine in the client-side
            // namespace — fold through the client resolver, which
            // returns the server CSWSObject* directly.
            creature = acc::engine::ResolveClientObjectHandle(handle);
        }
        if (!creature || creature == leader) continue;
        void* round = ReadCombatRound(creature);
        if (!round) continue;
        int local = CountQueueEntries(round);
        if (local <= 0) continue;

        char name[64] = "";
        ResolveMemberName(handle, name, sizeof(name));

        for (int i = 0; i < local && g_state.count < kMaxRows; ++i) {
            Row& r = g_state.rows[g_state.count++];
            r.creature = creature;
            r.combatRound = round;
            r.perCreatureIdx = i;
            r.perCreatureCount = local;
            std::strncpy(r.charName, name, sizeof(r.charName) - 1);
            r.charName[sizeof(r.charName) - 1] = '\0';
        }
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

    // Default: localized verb keyed off action_type, with the SPECIFIC
    // ability name swapped in for type=9/10/11 (see BuildActionLabel).
    // type=1 (Attack), 6/7 (Equip/Unequip) keep the generic verb — there
    // isn't a single specific-ability name per entry there.
    char verbBuf[96] = "";
    BuildActionLabel(action, verbBuf, sizeof(verbBuf));
    const char* verb = verbBuf;

    char tgtName[64] = "";
    if (target != 0u && target != 0x7F000000u) {
        if (!acc::engine::GetObjectDisplayNameByHandle(
                target, tgtName, sizeof(tgtName)) ||
            tgtName[0] == '\0') {
            void* tgtObj = acc::engine::ResolveServerObjectHandle(target);
            if (tgtObj) {
                acc::engine::GetObjectName(tgtObj, tgtName, sizeof(tgtName));
            }
        }
        if (tgtName[0] != '\0') {
            void* tgtObj = acc::engine::ResolveServerObjectHandle(target);
            if (tgtObj) {
                acc::narration::AppendSuffix(tgtObj, tgtName, sizeof(tgtName));
            }
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

// Try to remove the action at row `idx`. Tail-only — the engine's
// only public per-round primitive is RemoveLastAction. Returns true
// on dispatch success.
bool RemoveRow(int idx) {
    if (idx < 0 || idx >= g_state.count) return false;
    const Row& row = g_state.rows[idx];
    // Tail-only — see docs/combat-system.md Phase 3 "Clear one" item.
    // We compare against the per-creature index so that the user's
    // current row maps to the tail of *that creature's* queue (not
    // the flat-list tail).
    if (row.perCreatureIdx != row.perCreatureCount - 1) {
        acclog::Write("Combat.Queue",
                      "remove flat=%d char=[%s] local=%d/%d -> non-tail "
                      "remove not implemented",
                      idx, row.charName, row.perCreatureIdx,
                      row.perCreatureCount);
        return false;
    }
    __try {
        auto fn = reinterpret_cast<PFN_RemoveLastAction>(
            kAddrCombatRoundRemoveLastAction);
        fn(row.combatRound);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Clear every party member's queue via repeated RemoveLastAction calls
// against each row's combat round.
bool ClearAllRows() {
    int total = g_state.count;
    int removed = 0;
    // Iterate rows back-to-front so each per-creature tail-remove
    // matches the engine's only primitive.
    for (int i = total - 1; i >= 0; --i) {
        __try {
            auto fn = reinterpret_cast<PFN_RemoveLastAction>(
                kAddrCombatRoundRemoveLastAction);
            fn(g_state.rows[i].combatRound);
            ++removed;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            break;
        }
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
            // Disarm first so EndOverlayPause clears OUR pause-owner bit. If the
            // unified action menu is still open underneath (the common Shift+H-
            // from-the-menu flow), it keeps holding the world pause, so the world
            // stays frozen — closing the queue returns the user to the menu
            // seamlessly rather than resuming the world. Re-announce where they
            // land instead of speaking "closed" (which would imply the whole
            // queueing session ended).
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
    // Module-load latch — BuildRows iterates party server creatures via
    // engine accessors; can't trust those handles mid-handoff.
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
    // Default open hotkey: Shift+H (Action::CombatQueueOpen).
    if (!acc::hotkeys::Pressed(acc::hotkeys::Action::CombatQueueOpen)) return;

    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    if (!Open()) return;
}

}  // namespace acc::combat::queue
