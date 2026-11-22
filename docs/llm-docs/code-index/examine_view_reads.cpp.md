# examine_view_reads.cpp (179 lines)

Source: `patches/Accessibility/examine_view_reads.cpp`

Reading a creature for the examine panel: hit points, level, faction, flags, and the names of feats and spells.

## Declarations (in source order)

- L30 `int CallIntThis(void* this_, uintptr_t addr)`: The engine's side-effect-free int accessors; -1 when the call faults.
- L39 `int CallIntThisInt(void* this_, int arg, uintptr_t addr)`
- L48 `bool CallFlag(void* serverCreature, uintptr_t addr)`
- L53 `bool IsCreature(void* obj)`
- L60 `bool IsSentinel(uint32_t handle)`
- L64 `void* ReadCreatureStats(void* serverCreature)`
- L72 `int ReadHpMax(void* serverCreature)`
- L79 `int ReadLevel(void* serverCreature)`
- L90 `int ReadDamageLevel(void* obj)`
- L97 `bool ReadBlindFlag(void* serverCreature)`: No invisible flag on purpose: CSWSCreature::GetInvisible is not a pure read (its Sanctuary branch rolls a save and changes perception), and the effect rows already name an active invisibility read-only.
- L101 `int ReadFactionId(void* serverCreature)`
- L108 `acc::strings::Id FactionWordIdFor(int factionId)`
- L132 `int Read2DDistanceMeters(void* obj)`
- L141 `uint32_t ReadLastTargetHandle()`
- L151 `bool IsCreatureDead(void* serverCreature)`
- L158 `bool IsHostileCreature(void* serverObject)`: The faction table of the examine panel's Faction row, so "hostile" means the same everywhere.
- L164 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`
- L171 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`
