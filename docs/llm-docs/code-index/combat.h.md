# combat.h (45 lines)

Source: `patches/Accessibility/combat.h`

Combat event narration.

Channels: - Combat mode entry/exit — debounced via the turn_announce stability pattern; speaks "Kampf beginnt" / "Kampf beendet". - Combat log — live narration via the OnAppendToMsgBuffer hook (see the msg_router subscriber at the bottom of combat.cpp). TickCombatLog polls CSWGuiInGameMessages.messages_listbox for diagnostic logging only; speech is OFF there because the engine fills that listbox lazily when the review screen opens.

Each Tick is cheap and idle when nothing is happening.

## Declarations (in source order)

- L25 `bool IsCombatActive()`: Cross-feature gating read — false on chain fault (treat as not-in-combat).
- L26 `bool IsPartyInCombat()`
- L28 `void TickCombatMode()`
- L29 `void TickCombatLog()`
- L33 `void TickCombatAbsorb()`: Flushes a debounced burst of damage-absorption lines into one spoken total.
- L38 `void TickCombatDeflect()`: Flushes debounced blaster-deflection bursts into one spoken line per deflector (" reflektiert N Schüsse").
- L42 `void TickCombatEffects()`: Flushes debounced per-target ability/grenade/force-power effects into one merged spoken line each.
