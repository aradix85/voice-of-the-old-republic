# examine_view.h (70 lines)

Source: `patches/Accessibility/examine_view.h`

Examine list view.

Synthetic in-DLL arrow-navigable list of pre-composed rows. KOTOR 1's CSWGuiExamine is a generic TLK message box, not a rich examine panel, so we build our own from direct field reads + engine name accessors.

User contract while armed: Open     — captures cycle/LastTarget focus, builds rows, speaks opener + row 0. Up/Down  — step through rows (Name, Faction, HP, Distance, Weapon, Effects×N, Feats×M). HP and distance refresh on step. Enter    — close (read-only, no commit). Esc      — close.

Self-disarms when the target becomes unresolvable.

Input routing lives in input_poll_router.cpp.

## Declarations (in source order)

- L27 `const char* EffectName(int type)`: Localized EFFECT_TYPES → display name.
- L34 `const char* EffectIconName(int iconId)`: Localized effecticon.2da row → display name — the names behind the sighted buff/debuff icon row on the portrait (the engine mirrors each EFFECTICON effect into the creature's effect-icon array, one icon per row id).
- L41 `bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize)`: Look up a feat's localized name by feat_index (CSWFeat[] index, ushort).
- L47 `bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize)`: Look up a spell / force power's localized name by spell_id (int).
- L52 `bool IsHostileCreature(void* serverObject)`: True iff serverObject is a Creature whose faction classifies as hostile (same faction table as the examine Faction row).
- L57 `bool IsCreatureDead(void* serverCreature)`: True iff the engine's CSWSCreature::GetDead reports the creature dead.
- L59 `bool Open()`
- L60 `bool IsActive()`
- L63 `bool HandleInputEvent(int code, int value)`: Press-edge only (value != 0).
- L65 `void ForceDisarm(const char* reason)`
- L66 `void Tick()`
- L67 `void PollWin32Hotkey()`
