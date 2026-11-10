# combat_log_attack.cpp (246 lines)

Source: `patches/Accessibility/combat_log_attack.cpp`

The attack sequence as one spoken line: collecting the summary and breakdown lines, then the compact, outgoing or incoming line, and the urgent status line.

## Declarations (in source order)

- L16 `void BuildCompact(const AttackBlock& b, char* out, size_t cap)`
- L88 `void BuildResultTail(const AttackBlock& b, char* tail, size_t cap)`
- L106 `void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap)`: The party's own attack.
- L128 `void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap)`: Incoming hit on a party member: " : from [, ...]".
- L152 `void FlushPending()`: Filter + emit decision for a fully-buffered attack block.
- L190 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: A status condition applied to a party member (stunned, poisoned, paralysed, feared — whatever the engine echoes in " is ") is a disabling state the player needs to act on, so it shouldn't wait behind queued combat cha...
- L202 `bool RuleSummary(const char* text)`
- L210 `bool RuleAttack(const char* text)`
- L215 `bool RuleDefense(const char* text)`
- L220 `bool RuleDamage(const char* text)`
- L232 `bool RuleThreat(const char* text)`: Bedrohungsstatistik is the critical-hit *confirmation* roll.
- L238 `void OnUnmatched(const char* /*text*/)`
