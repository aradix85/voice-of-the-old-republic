# combat.cpp (147 lines)

Source: `patches/Accessibility/combat.cpp`

Combat mode: "Combat begins", "Combat ends" with who died and how the party stands, and the leader at peace while the party still fights.

## Declarations (in source order)

- L31 `bool ReadCombatMode(int& outMode)`
- L51 `void Announce(const char* phrase)`
- L56 `void Announce(acc::strings::Id id)`
- L63 `void AnnounceEnd()`: "Combat ends", after a fight with kills followed by who died and how the party stands.
- L84 `void NoteKillLine(const char* line)`
- L89 `bool IsCombatActive()`
- L94 `bool IsPartyInCombat()`
- L98 `void TickCombatMode()`
