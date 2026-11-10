# examine_view_reads.cpp (347 lines)

Source: `patches/Accessibility/examine_view_reads.cpp`

Reading a creature for the examine panel: hit points, level, faction, flags, equipment, and the names of feats and spells.

## Declarations (in source order)

- L28 `void* GetCSWRules()`
- L37 `int CallIntThis(void* this_, uintptr_t addr)`
- L47 `int CallIntThisInt(void* this_, int arg, uintptr_t addr)`
- L57 `bool ReadDeadFlag(void* serverCreature)`
- L64 `bool IsSentinel(uint32_t handle)`
- L68 `void* ReadCreatureStats(void* serverCreature)`
- L79 `int ReadHpMax(void* serverCreature)`
- L87 `int ReadLevel(void* serverCreature)`
- L102 `int ReadDamageLevel(void* obj)`
- L117 `bool ReadBlindFlag(void* serverCreature)`
- L122 `acc::strings::Id DamageLevelStringId(int level)`
- L135 `int ReadFactionId(void* serverCreature)`
- L149 `acc::strings::Id FactionWordIdFor(int factionId)`
- L173 `int Read2DDistanceMeters(void* obj)`
- L187 `bool ReadEquippedItemNameAtSlot(void* serverCreature, size_t slotOffset, char* outBuf, size_t outBufSize)`: Read an equipped item's display name by walking the inventory ulong handle at the given slot offset (kInventory*HandleOffset).
- L224 `uint32_t ReadLastTargetHandle()`
- L242 `bool IsCreatureDead(void* serverCreature)`: Public — true iff serverObject is a Creature whose faction classifies as hostile.
- L251 `bool IsHostileCreature(void* serverObject)`
- L270 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`: Public — same shape as ResolveFeatName below, but for the spells array.
- L313 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`: Public — declared in examine_view.h.
