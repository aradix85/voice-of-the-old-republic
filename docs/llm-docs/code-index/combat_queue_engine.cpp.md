# combat_queue_engine.cpp (372 lines)

Source: `patches/Accessibility/combat_queue_engine.cpp`

Reading the engine's combat rounds: the queued actions, their kind and target, and the label a queued action is spoken by.

## Declarations (in source order)

- L24 `void* NodeAction(void* node)`
- L29 `void* NodeNext(void* node)`
- L36 `bool ReadNodeActionType(void* node, unsigned char& outType)`: The node's action type, under its own SEH so one bad node is skipped rather than ending the walk.
- L51 `void* ListInternal(void* combatRound)`: The list's internal block, or nullptr.
- L62 `void* WalkQueue(void* combatRound, int index, int* outCount)`: Walks the real (non-placeholder) entries.
- L89 `unsigned short ReadActionFeatId(void* action)`: The ushort feat id of a type-11 (Use Feat) action, which CSWGuiMainInterface::GetActionIcon feeds to CSWRules::GetFeat; 0xFFFF on a fault.
- L97 `int ReadActionSpellId(void* action)`: The spell id of a type-9 (Cast Force Power) action, which GetActionIcon feeds to CSWSpellArray::GetSpell; -1 on a fault.
- L104 `uint32_t ReadActionItemHandle(void* action)`: The server item handle of a type-10 (Use Item) action.
- L112 `uint32_t ReadFirstQueuedTarget(void* serverCreature)`: Target (server id) of the first real action queued on the creature's combat round; 0 when nothing is queued or the read faults.
- L152 `void* GetPlayerCombatRound()`: Resolve the player's CSWSCombatRound at hook-fire time so we can tag each ADD/CLEAR event with whether it belongs to the user's creature (vs companion / enemy combat rounds, which also use the same engine surfaces).
- L174 `int ReadRoundActionCount(void* combatRound)`: Queued-action count on a CSWSCombatRound directly.
- L198 `int ReadQueueSize(void* serverCreature)`
- L219 `int ReadOuterQueueSize(void* serverCreature)`: Read the OUTER game-action queue size at CSWSObject.action_nodes @+0xfc.
- L240 `bool LeaderIsAttacking(uint32_t targetId)`
- L247 `const char* RoleTag(void* combatRound)`
- L251 `void* ReadCombatRound(void* serverCreature)`
- L257 `int CountQueueEntries(void* combatRound)`
- L263 `void* GetQueueAction(void* combatRound, int index)`
- L272 `acc::strings::Id VerbForActionType(unsigned char actionType)`: The action types decoded by CSWGuiMainInterface::GetActionIcon (0x686fb0), the engine's own per-slot icon resolver: 1 attack (a door bash is an attack on a door), 6 equip, 7 unequip, 9 cast Force power, 10 use item, 1...
- L284 `bool ReadActionFields(void* action, unsigned char& outType, uint32_t& outTarget)`
- L304 `void BuildActionLabel(void* action, char* out, size_t n)`: The specific Force power, item or feat name for types 9, 10 and 11, else the generic verb.
- L341 `int RawRoundCount(void* combatRound)`
- L364 `int CountPlayerEntries()`: The engine's own count, placeholder included: the value AddAction's cap check (`if (3 < count)`) inspects, so "position N" and "queue full" match the engine's slots.
