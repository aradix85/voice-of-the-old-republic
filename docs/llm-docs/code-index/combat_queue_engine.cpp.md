# combat_queue_engine.cpp (337 lines)

Source: `patches/Accessibility/combat_queue_engine.cpp`

Reading the engine's combat rounds: the queued actions, their kind and target, and the label a queued action is spoken by.

## Declarations (in source order)

- L32 `bool ReadNodeActionType(void* node, unsigned char& outType)`
- L51 `unsigned short ReadActionFeatId(void* action)`: Read the feat_id stored on a type=11 (Use Feat) action node.
- L65 `int ReadActionSpellId(void* action)`: Read the spell_id stored on a type=9 (Cast Force Power) action node.
- L79 `uint32_t ReadActionItemHandle(void* action)`: Read the item handle stored on a type=10 (Use Item) action node.
- L93 `void* ReadCombatRound(void* serverCreature)`: Read CSWSCreature.combat_round @+0x9c8.
- L104 `int CountQueueEntries(void* combatRound)`
- L140 `void* GetQueueAction(void* combatRound, int index)`: Get the action node at zero-based `index` (skipping placeholder type=0xFF entries), or nullptr if out of range.
- L196 `acc::strings::Id VerbForActionType(unsigned char actionType)`: Map an action_type byte to a localised verb.
- L209 `bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget)`: Read action_type byte + target handle from a CSWSCombatRoundAction.
- L235 `void BuildActionLabel(void* action, char* out, size_t n)`: Build the spoken label for a queued action node: the SPECIFIC ability name for force powers / items / feats (type 9/10/11), else the generic verb ("Attack" / "Use Feat" / ...).
- L277 `int RawRoundCount(void* combatRound)`: Read the raw queue count from a combat round (engine's own actions->internal->count — the value AddAction's `if (3 < count)` cap inspects).
- L298 `int CountPlayerEntries()`
