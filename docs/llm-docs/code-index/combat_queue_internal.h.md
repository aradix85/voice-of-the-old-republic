# combat_queue_internal.h (38 lines)

Source: `patches/Accessibility/combat_queue_internal.h`

Internal to the combat_queue module (combat_queue*.cpp); no other file includes it. The public interface is combat_queue.h.

## Declarations (in source order)

- L21 `void* ReadCombatRound(void* serverCreature)`: CSWSCreature.combat_round, or nullptr.
- L24 `int CountQueueEntries(void* combatRound)`: The round's queued actions, without the engine's leading placeholder.
- L25 `void* GetQueueAction(void* combatRound, int index)`
- L28 `int RawRoundCount(void* combatRound)`: The round's raw count, placeholder included; -1 on a fault.
- L30 `acc::strings::Id VerbForActionType(unsigned char actionType)`
- L31 `bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget)`
- L35 `void BuildActionLabel(void* action, char* out, size_t n)`: How a queued action is spoken, the same for the Shift+H review and the live announce.
