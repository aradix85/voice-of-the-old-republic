# combat_log_attack.cpp (210 lines)

Source: `patches/Accessibility/combat_log_attack.cpp`

The attack sequence as one spoken line: collecting the summary and breakdown lines, then the outgoing or incoming line, and the urgent status line.

## Declarations (in source order)

- L19 `void BuildCompact(const AttackBlock& b, char* out, size_t cap)`: The whole block on one line, for the log of an attack that is not spoken.
- L53 `void BuildResultTail(const AttackBlock& b, char* tail, size_t cap)`
- L70 `void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap)`: The party's own attack.
- L92 `void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap)`: Incoming hit on a party member: " : from [, ...]".
- L116 `void FlushPending()`: Filter + emit decision for a fully-buffered attack block.
- L154 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: A status condition applied to a party member (stunned, poisoned, paralysed, feared — whatever the engine echoes in " is ") is a disabling state the player needs to act on, so it shouldn't wait behind queued combat cha...
- L166 `bool RuleSummary(const char* text)`
- L174 `bool RuleAttack(const char* text)`
- L179 `bool RuleDefense(const char* text)`
- L184 `bool RuleDamage(const char* text)`
- L196 `bool RuleThreat(const char* text)`: The Threat Breakdown is the critical-hit *confirmation* roll.
- L202 `void OnUnmatched(const char* /*text*/)`
