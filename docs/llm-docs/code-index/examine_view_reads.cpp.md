# examine_view_reads.cpp (230 lines)

Source: `patches/Accessibility/examine_view_reads.cpp`

Reading a creature for the examine panel: hit points, level, faction, flags, and the names of feats and spells.

## Declarations (in source order)

- L31 `void* GetCSWRules()`: CSWRules sits at offset 0 of CSWSRules: the same pointer.
- L38 `int CallIntThis(void* this_, uintptr_t addr)`: The engine's side-effect-free int accessors; -1 when the call faults.
- L47 `int CallIntThisInt(void* this_, int arg, uintptr_t addr)`
- L56 `bool CallFlag(void* serverCreature, uintptr_t addr)`
- L61 `bool IsCreature(void* obj)`
- L68 `bool ReadRowName(void* row, size_t offset, char* outBuf, size_t outBufSize)`: The name strref at `offset` of a rules row, through the dual-game TLK path.
- L77 `bool IsSentinel(uint32_t handle)`
- L81 `void* ReadCreatureStats(void* serverCreature)`
- L89 `int ReadHpMax(void* serverCreature)`
- L96 `int ReadLevel(void* serverCreature)`
- L107 `int ReadDamageLevel(void* obj)`
- L114 `bool ReadBlindFlag(void* serverCreature)`: No invisible flag on purpose: CSWSCreature::GetInvisible is not a pure read (its Sanctuary branch rolls a save and changes perception), and the effect rows already name an active invisibility read-only.
- L118 `acc::strings::Id DamageLevelStringId(int level)`
- L131 `int ReadFactionId(void* serverCreature)`
- L138 `acc::strings::Id FactionWordIdFor(int factionId)`
- L162 `int Read2DDistanceMeters(void* obj)`
- L172 `uint32_t ReadLastTargetHandle()`
- L182 `bool IsCreatureDead(void* serverCreature)`
- L189 `bool IsHostileCreature(void* serverObject)`: The faction table of the examine panel's Faction row, so "hostile" means the same everywhere.
- L196 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`: Rules->spells (CSWSpellArray), GetSpell(spellId), then the row's name.
- L215 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`: CSWRules::GetFeat(featIdx), then the row's name (offset 0x08 in both games).
