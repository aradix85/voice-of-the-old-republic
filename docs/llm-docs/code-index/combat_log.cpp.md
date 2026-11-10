# combat_log.cpp (126 lines)

Source: `patches/Accessibility/combat_log.cpp`

The combat log as speech: the rules that turn the engine's combat-log lines into the mod's short spoken lines, registered with acc::msg::Router. Here: the registry, the shared text helpers, and the one-line rules (a kill, a detected mine).

## Declarations (in source order)

- L22 `bool RuleKill(const char* text)`: " killed : N XP" — a defeat.
- L39 `bool RuleMineDetect(const char* text)`: Mine-detection feedback enrichment.
- L90 `bool MsgStartsWith(const char* s, const char* p)`
- L95 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L104 `void RegisterCombatMsgRules()`
