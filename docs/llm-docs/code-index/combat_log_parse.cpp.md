# combat_log_parse.cpp (360 lines)

Source: `patches/Accessibility/combat_log_parse.cpp`

Parsing the engine's attack sequence: the summary line and the attack, defense and damage breakdowns, into one AttackSeq.

## Declarations (in source order)

- L14 `void ParseSummaryTail(const char* text, const char* with_at, AttackBlock& b)`: Numeric tail of the attack summary: the attack / defence / damage values behind phrase_with, and the crit and auto tags.
- L33 `bool ParseSummaryHead(const char* text, AttackBlock& b)`: Head of the attack summary: " .
- L66 `bool ParseSummary(const char* text, AttackBlock& b)`
- L94 `bool ParseAttack(const char* text, AttackBlock& b)`
- L195 `bool ParseDefense(const char* text, AttackBlock& b)`
- L233 `bool ParseDamage(const char* text, AttackBlock& b)`
