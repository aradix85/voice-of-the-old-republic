# combat_log_shields.cpp (206 lines)

Source: `patches/Accessibility/combat_log_shields.cpp`

Energy shields and blaster deflection: absorbed damage and deflected shots, collected and spoken once per burst.

## Declarations (in source order)

- L52 `void FlushAbsorb()`: Flush the pending absorb burst as one spoken total.
- L71 `void FlushDeflect(DeflectSlot& s)`: Flush one deflection slot as a single spoken line (" deflects N shots").
- L91 `bool RuleAbsorb(const char* text)`: Accumulate a damage-absorption line into the pending burst (claimed — no raw speech).
- L146 `bool RuleDeflect(const char* text)`: "Deflection Breakdown: deflects projectile with N = vs.
- L187 `void TickCombatAbsorb()`
- L195 `void TickCombatDeflect()`
