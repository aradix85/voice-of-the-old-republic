# combat.h (35 lines)

Source: `patches/Accessibility/combat.h`

Combat narration: the combat-mode cues (combat.cpp) and the flushes of the combat log's merged lines (combat_log_shields.cpp, combat_log_effects.cpp). The log itself is narrated from the OnAppendToMsgBuffer hook.

## Declarations (in source order)

- L16 `bool IsCombatActive()`: Cross-feature gating read — false on chain fault (treat as not-in-combat).
- L17 `bool IsPartyInCombat()`
- L19 `void TickCombatMode()`
- L23 `void TickCombatAbsorb()`: Flushes a debounced burst of damage-absorption lines into one spoken total.
- L28 `void TickCombatDeflect()`: Flushes debounced blaster-deflection bursts into one spoken line per deflector (" deflects N shots").
- L32 `void TickCombatEffects()`: Flushes debounced per-target ability/grenade/force-power effects into one merged spoken line each.
