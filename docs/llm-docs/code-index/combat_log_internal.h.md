# combat_log_internal.h (73 lines)

Source: `patches/Accessibility/combat_log_internal.h`

Internal to the combat_log module (combat_log*.cpp); no other file includes it. The public interface is combat.h. Here: what the files share.

## Declarations (in source order)

- L17 `struct AttackBlock`: One attack sequence as it is collected: it opens on a summary line and closes on the Damage Breakdown (hit) or on any unrecognised line (miss).
- L44 `bool MsgStartsWith(const char* s, const char* p)`: combat_log.cpp
- L45 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L48 `bool ParseSummary(const char* text, AttackBlock& b)`: combat_log_parse.cpp
- L49 `bool ParseAttack(const char* text, AttackBlock& b)`
- L50 `bool ParseDefense(const char* text, AttackBlock& b)`
- L51 `bool ParseDamage(const char* text, AttackBlock& b)`
- L54 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: combat_log_attack.cpp
- L55 `bool RuleSummary(const char* text)`
- L56 `bool RuleAttack(const char* text)`
- L57 `bool RuleDefense(const char* text)`
- L58 `bool RuleDamage(const char* text)`
- L59 `bool RuleThreat(const char* text)`
- L60 `void OnUnmatched(const char* )`
- L63 `bool RuleEffectApplication(const char* text)`: combat_log_effects.cpp
- L64 `bool RuleAbilityUse(const char* text)`
- L65 `bool RuleSaveThrow(const char* text)`
- L66 `bool RuleDirectDamage(const char* text)`
- L69 `bool RuleAbsorb(const char* text)`: combat_log_shields.cpp
- L70 `bool RuleDeflect(const char* text)`
