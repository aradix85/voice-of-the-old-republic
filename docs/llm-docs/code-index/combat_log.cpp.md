# combat_log.cpp (134 lines)

Source: `patches/Accessibility/combat_log.cpp`

The combat log as speech: the rules, registered with acc::msg::Router, that turn the engine's combat-log lines into the mod's short spoken lines. The 3-or-4-line attack sequence (summary, Attack Breakdown, [Threat Breakdown], Defense Breakdown, [Damage Breakdown]) becomes one compact line (combat_log_attack.cpp); abilities, shields and deflection have their own files. Everything else (heals, item use, loot, world feedback) falls through to the router's raw-speech fallback. Here: the registry, the shared text helpers, and the one-line rules (a kill, a detected mine).

Parsing is anchor-based against the engine strings in combat_strings.*; the anchors are exact (spacing and punctuation included) and must not be "tidied".

## Declarations (in source order)

- L32 `bool RuleKill(const char* text)`: " killed : N XP" — a defeat.
- L49 `bool RuleMineDetect(const char* text)`: Mine-detection feedback enrichment.
- L98 `bool MsgStartsWith(const char* s, const char* p)`
- L103 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L112 `void RegisterCombatMsgRules()`
