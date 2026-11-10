# examine_view_reads.cpp (192 lines)

Source: `patches/Accessibility/examine_view_reads.cpp`

Reading a creature for the examine panel: hit points, level, faction, flags, and the names of feats and spells.

## Declarations (in source order)

- L29 `int CallIntThis(void* this_, uintptr_t addr)`: The engine's side-effect-free int accessors; -1 when the call faults.
- L38 `int CallIntThisInt(void* this_, int arg, uintptr_t addr)`
- L47 `bool CallFlag(void* serverCreature, uintptr_t addr)`
- L52 `bool IsCreature(void* obj)`
- L59 `bool IsSentinel(uint32_t handle)`
- L63 `void* ReadCreatureStats(void* serverCreature)`
- L71 `int ReadHpMax(void* serverCreature)`
- L78 `int ReadLevel(void* serverCreature)`
- L89 `int ReadDamageLevel(void* obj)`
- L96 `bool ReadBlindFlag(void* serverCreature)`: No invisible flag on purpose: CSWSCreature::GetInvisible is not a pure read (its Sanctuary branch rolls a save and changes perception), and the effect rows already name an active invisibility read-only.
- L100 `acc::strings::Id DamageLevelStringId(int level)`
- L113 `int ReadFactionId(void* serverCreature)`
- L120 `acc::strings::Id FactionWordIdFor(int factionId)`
- L144 `int Read2DDistanceMeters(void* obj)`
- L154 `uint32_t ReadLastTargetHandle()`
- L164 `bool IsCreatureDead(void* serverCreature)`
- L171 `bool IsHostileCreature(void* serverObject)`: The faction table of the examine panel's Faction row, so "hostile" means the same everywhere.
- L177 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`
- L184 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`
