# combat_log_shields.cpp (208 lines)

Source: `patches/Accessibility/combat_log_shields.cpp`

Energy shields and blaster deflection: absorbed damage and deflected shots, collected and spoken once per burst.

## Declarations (in source order)

- L54 `void FlushAbsorb()`: Flush the pending absorb burst as one spoken total.
- L73 `void FlushDeflect(DeflectSlot& s)`: Flush one deflection slot as a single spoken line (" deflects N shots").
- L93 `bool RuleAbsorb(const char* text)`: Accumulate a damage-absorption line into the pending burst (claimed — no raw speech).
- L148 `bool RuleDeflect(const char* text)`: "Deflection Breakdown: deflects projectile with N = vs.
- L189 `void TickCombatAbsorb()`
- L197 `void TickCombatDeflect()`
