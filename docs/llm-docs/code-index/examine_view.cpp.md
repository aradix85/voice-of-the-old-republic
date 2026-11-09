# examine_view.cpp (773 lines)

Source: `patches/Accessibility/examine_view.cpp`

(From examine_view.h.) Ö examine list view.

Synthetic in-DLL arrow-navigable list of pre-composed rows. KOTOR 1's CSWGuiExamine is a generic TLK message box, not a rich examine panel, so we build our own from direct field reads + engine name accessors.

User contract while armed: Open     — captures cycle/LastTarget focus, builds rows, speaks opener + row 0. Up/Down  — step through rows (Name, Faction, HP, Distance, Weapon, Effects×N, Feats×M). HP and distance refresh on step. Enter    — close (read-only, no commit). Esc      — close.

Self-disarms when the target becomes unresolvable.

Input routing lives in interact_hotkey.cpp.

## Declarations (in source order)

- L44 `void* GetCSWRules()`
- L56 `bool IsSentinel(uint32_t handle)`
- L60 `void* ReadCreatureStats(void* serverCreature)`
- L75 `int CallIntThis(void* this_, uintptr_t addr)`
- L85 `int CallIntThisInt(void* this_, int arg, uintptr_t addr)`
- L95 `int ReadHpMax(void* serverCreature)`
- L103 `int ReadLevel(void* serverCreature)`
- L118 `int ReadDamageLevel(void* obj)`
- L123 `bool ReadDeadFlag(void* serverCreature)`
- L138 `bool ReadBlindFlag(void* serverCreature)`
- L143 `acc::strings::Id DamageLevelStringId(int level)`
- L156 `int ReadFactionId(void* serverCreature)`
- L170 `acc::strings::Id FactionWordIdFor(int factionId)`
- L194 `int Read2DDistanceMeters(void* obj)`
- L208 `bool ReadEquippedItemNameAtSlot(void* serverCreature, size_t slotOffset, char* outBuf, size_t outBufSize)`: Read an equipped item's display name by walking the inventory ulong handle at the given slot offset (kInventory*HandleOffset).
- L248 `int AppendEffectRows(void* serverObject, char rows[][192], int& outIdx, int rowCap)`: Walk CSWSObject.effects (CExoArrayList @+0x124) and emit rows for up to `cap` effects.
- L287 `int AppendFeatRows(void* serverCreature, char rows[][192], int& outIdx, int rowCap)`: Walk CSWSCreatureStats.feats (CExoArrayList @+0x0) and emit rows for up to `cap` feats with localized names from CSWRules::GetFeat.
- L328 `uint32_t ReadLastTargetHandle()`
- L344 `int BuildRows()`: (Re)build the row list for the cached target.
- L508 `void SpeakRow(int idx)`
- L525 `bool IsCreatureDead(void* serverCreature)`: Public — true iff serverObject is a Creature whose faction classifies as hostile.
- L534 `bool IsHostileCreature(void* serverObject)`
- L553 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`: Public — same shape as ResolveFeatName below, but for the spells array.
- L596 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`: Public — declared in examine_view.h.
- L630 `bool IsActive()`
- L632 `void ForceDisarm(const char* reason)`
- L644 `bool Open()`
- L693 `bool HandleInputEvent(int code, int value)`
- L743 `void Tick()`: Engine-panel open/close logger.
- L754 `void PollWin32Hotkey()`
