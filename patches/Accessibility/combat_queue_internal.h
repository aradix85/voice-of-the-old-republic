// Internal to the combat_queue module (combat_queue*.cpp); no other file includes
// it. The public interface is combat_queue.h. Here: what the files share.

#pragma once

#include "combat_queue.h"

#include <windows.h>  // GetTickCount — user-attribution freshness window

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "strings.h"

namespace acc::combat::queue {

// Flat row model.
// One row per queued action, across every party member's combat round.
// Built at Open() and rebuilt after every mutation (remove / clear).
//
// The controlled creature is added first via GetPlayerServerCreature
// (the same path the "position N" announce uses). The remaining party
// members come from CSWPartyTable.pt_member_ids via GetPartyMembers —
// those are NPC *roster* slots (companions); the PC is not a roster NPC
// and never appears there, which is why the player's own queue must be
// sourced separately (see BuildRows).
struct Row {
    void*    creature;     // CSWSCreature* — owner of this action
    void*    combatRound;  // CSWSCombatRound* — for tail-remove dispatch
    int      perCreatureIdx;  // 0-based index of this row within the
                              // owner's local queue (used for the count
                              // we tail-remove against)
    int      perCreatureCount;  // total queued on this owner (snapshot)
    char     charName[64];
};

constexpr int kMaxRows = 32;  // 3 members × up to ~10 actions each, with
                              // headroom for transient additions

// Helpers shared across the files, by the file that defines them.

// combat_queue_engine.cpp
void* ReadCombatRound(void* serverCreature);
int CountQueueEntries(void* combatRound);
void* GetQueueAction(void* combatRound, int index);
acc::strings::Id VerbForActionType(unsigned char actionType);
bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget);
void BuildActionLabel(void* action, char* out, size_t n);
int RawRoundCount(void* combatRound);

}  // namespace acc::combat::queue
