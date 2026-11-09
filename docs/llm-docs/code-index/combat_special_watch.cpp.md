# combat_special_watch.cpp (345 lines)

Source: `patches/Accessibility/combat_special_watch.cpp`

GameObjectKind

## Declarations (in source order)

- L67 `void* ReadCombatRound(void* serverCreature)`: Read CSWSCreature.combat_round @+0x9c8.
- L84 `bool IsRoutineAutoAttack(void* action)`: True if `action` represents a routine auto-attack that the engine would have queued itself once combat is engaged.
- L131 `int CountSpecialsForCreature(void* creature, const char* tag, acclog::BlockLog* dump)`: Walk one creature's combat_round.actions, returning the count of "special" actions (everything that ISN'T a routine auto-attack against a hostile creature).
- L198 `int CountPartySpecials()`: Party-wide specials count.
- L241 `State& GetState()`
- L246 `void ResetForExit(State& s)`
- L253 `void FireCue(const char* reason, int specials, DWORD now)`
- L280 `void Tick()`
