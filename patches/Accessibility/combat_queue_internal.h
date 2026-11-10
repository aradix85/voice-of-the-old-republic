// Internal to the combat_queue module (combat_queue*.cpp); no other file
// includes it. The public interface is combat_queue.h.

#pragma once

#include "combat_queue.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "strings.h"

namespace acc::combat::queue {

// combat_queue_engine.cpp

// CSWSCreature.combat_round, or nullptr.
void* ReadCombatRound(void* serverCreature);

// The round's queued actions, without the engine's leading placeholder.
int CountQueueEntries(void* combatRound);
void* GetQueueAction(void* combatRound, int index);

acc::strings::Id VerbForActionType(unsigned char actionType);
bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget);

// How a queued action is spoken, the same for the Shift+H review and the
// live announce.
void BuildActionLabel(void* action, char* out, size_t n);

}  // namespace acc::combat::queue
