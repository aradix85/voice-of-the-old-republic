# combat.cpp (101 lines)

Source: `patches/Accessibility/combat.cpp`

Combat mode: "Combat begins", "Combat ends" and the leader at peace while the party still fights.

## Declarations (in source order)

- L26 `bool ReadCombatMode(int& outMode)`
- L40 `void Announce(acc::strings::Id id)`
- L48 `bool IsCombatActive()`
- L53 `bool IsPartyInCombat()`
- L57 `void TickCombatMode()`
