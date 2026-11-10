# combat_log_internal.h (92 lines)

Source: `patches/Accessibility/combat_log_internal.h`

Internal to the combat_log module (combat_log*.cpp); no other file includes it. The public interface is combat.h. Here: what the files share.

## Declarations (in source order)

- L36 `struct AttackBlock`
- L63 `bool MsgStartsWith(const char* s, const char* p)`: combat_log.cpp
- L64 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L67 `bool ParseSummary(const char* text, AttackBlock& b)`: combat_log_parse.cpp
- L68 `bool ParseAttack(const char* text, AttackBlock& b)`
- L69 `bool ParseDefense(const char* text, AttackBlock& b)`
- L70 `bool ParseDamage(const char* text, AttackBlock& b)`
- L73 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: combat_log_attack.cpp
- L74 `bool RuleSummary(const char* text)`
- L75 `bool RuleAttack(const char* text)`
- L76 `bool RuleDefense(const char* text)`
- L77 `bool RuleDamage(const char* text)`
- L78 `bool RuleThreat(const char* text)`
- L79 `void OnUnmatched(const char* )`
- L82 `bool RuleEffectApplication(const char* text)`: combat_log_effects.cpp
- L83 `bool RuleAbilityUse(const char* text)`
- L84 `bool RuleSaveThrow(const char* text)`
- L85 `bool RuleDirectDamage(const char* text)`
- L88 `bool RuleAbsorb(const char* text)`: combat_log_shields.cpp
- L89 `bool RuleDeflect(const char* text)`
