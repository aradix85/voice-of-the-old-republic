# combat_queue_internal.h (53 lines)

Source: `patches/Accessibility/combat_queue_internal.h`

Internal to the combat_queue module (combat_queue*.cpp); no other file includes it. The public interface is combat_queue.h. Here: what the files share.

## Declarations (in source order)

- L28 `struct Row`: Flat row model.
- L44 `void* ReadCombatRound(void* serverCreature)`: combat_queue_engine.cpp
- L45 `int CountQueueEntries(void* combatRound)`
- L46 `void* GetQueueAction(void* combatRound, int index)`
- L47 `acc::strings::Id VerbForActionType(unsigned char actionType)`
- L48 `bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget)`
- L49 `void BuildActionLabel(void* action, char* out, size_t n)`
- L50 `int RawRoundCount(void* combatRound)`

Also 1 constants or variables at namespace scope.
