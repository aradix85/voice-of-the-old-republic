# combat_log_effects.cpp (295 lines)

Source: `patches/Accessibility/combat_log_effects.cpp`

Abilities, grenades and Force powers: who used what, the saving throws and damage per target, merged into one line per use.

## Declarations (in source order)

- L49 `EffectTarget* FxFind(const char* target)`
- L58 `EffectTarget* FxAlloc(const char* target)`: Find-or-allocate a slot for target.
- L80 `void BuildEffectLine(const EffectTarget& e, char* out, size_t cap)`: Build one merged per-target effect line.
- L130 `void FlushEffect(EffectTarget& e)`: Speak (and clear) one accumulated effect slot.
- L167 `bool RuleEffectApplication(const char* text)`: "Effect Application Breakdown: is " — the engine's trailing status echo.
- L188 `bool RuleAbilityUse(const char* text)`: " uses [.
- L221 `bool RuleSaveThrow(const char* text)`: " Save.
- L248 `bool RuleDirectDamage(const char* text)`: " damages : N damage (Type: N)" — direct/ability damage.
- L284 `void TickCombatEffects()`
