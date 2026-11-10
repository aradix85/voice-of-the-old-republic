# combat_log_parse.cpp (353 lines)

Source: `patches/Accessibility/combat_log_parse.cpp`

Parsing the engine's attack sequence: the summary line and the attack, defense and damage breakdowns, into one AttackBlock.

## Declarations (in source order)

- L15 `void ParseSummaryTail(const char* text, const char* with_at, AttackBlock& b)`: Numeric tail of the attack summary: the attack / defence / damage values behind phrase_with, and the crit and auto tags.
- L34 `bool ParseSummaryHead(const char* text, AttackBlock& b)`: Head of the attack summary: " .
- L67 `bool ParseSummary(const char* text, AttackBlock& b)`
- L95 `bool ParseAttack(const char* text, AttackBlock& b)`
- L188 `bool ParseDefense(const char* text, AttackBlock& b)`
- L226 `bool ParseDamage(const char* text, AttackBlock& b)`
