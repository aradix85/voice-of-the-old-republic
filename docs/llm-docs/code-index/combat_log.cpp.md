# combat_log.cpp (1242 lines)

Source: `patches/Accessibility/combat_log.cpp`

combat-log message parsing and the spoken result lines.

Split out of combat.cpp by the Phase-1 structure pass (refactoring candidate 7). combat.cpp keeps combat-mode tracking (TickCombatMode / TickCombatLog and the IsCombatActive / IsPartyInCombat predicates); everything here is the acc::msg::Router rule set that turns the engine's localized combat-log lines into the mod's short spoken announcements.

The two halves already lived in separate namespace blocks in the original file and share no state - the parser owns all of its own statics, and core_tick reaches the three tickers through combat.h. The move is verbatim, including this file's own extra include block below.

Parsing is anchor-based against the localized strings in combat_strings.*; see project_combat_loc_en_phrase_mit_bug for why the anchors are fragile and must not be "tidied".

## Declarations (in source order)

- L164 `EffectTarget* FxFind(const char* target)`
- L173 `EffectTarget* FxAlloc(const char* target)`: Find-or-allocate a slot for target.
- L195 `bool MsgStartsWith(const char* s, const char* p)`
- L200 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L212 `void ParseSummaryTail(const char* text, const char* mit_at, AttackBlock& b)`: Numeric tail of the attack summary: the attack / defence / damage values behind phrase_mit, and the crit and auto tags.
- L231 `bool ParseSummaryHead(const char* text, AttackBlock& b)`: Head of the attack summary: " .
- L262 `bool ParseSummary(const char* text, AttackBlock& b)`
- L290 `bool ParseAngriff(const char* text, AttackBlock& b)`
- L391 `bool ParseAbwehr(const char* text, AttackBlock& b)`
- L429 `bool ParseSchaden(const char* text, AttackBlock& b)`
- L555 `void BuildCompact(const AttackBlock& b, char* out, size_t cap)`
- L627 `void BuildResultTail(const AttackBlock& b, char* tail, size_t cap)`
- L646 `void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap)`: The party's own attack.
- L668 `void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap)`: Incoming hit on a party member: " : von [, ...]".
- L687 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: A status condition applied to a party member (stunned, poisoned, paralysed, feared — whatever the engine echoes in " ist ") is a disabling state the player needs to act on, so it shouldn't wait behind queued combat ch...
- L709 `void FlushPending()`: Filter + emit decision for a fully-buffered attack block.
- L740 `bool RuleSummary(const char* text)`
- L748 `bool RuleAngriff(const char* text)`
- L753 `bool RuleAbwehr(const char* text)`
- L758 `bool RuleSchaden(const char* text)`
- L772 `bool RuleAuswirkung(const char* text)`: "Auswirkungsstatistik: ist " — the engine's trailing status echo.
- L794 `bool RuleAbilityUse(const char* text)`: " benutzt [.
- L827 `bool RuleSaveThrow(const char* text)`: " -Rettungswurf.
- L854 `bool RuleDirectDamage(const char* text)`: " verletzt : N Schaden (Type: N)" — direct/ability damage.
- L891 `void BuildEffectLine(const EffectTarget& e, char* out, size_t cap)`: Build one merged per-target effect line.
- L941 `void FlushEffect(EffectTarget& e)`: Speak (and clear) one accumulated effect slot.
- L970 `void FlushAbsorb()`: Flush the pending absorb burst as one spoken total.
- L987 `bool RuleAbsorb(const char* text)`: Accumulate a damage-absorption line into the pending burst (claimed — no raw speech).
- L1040 `void FlushDeflect(DeflectSlot& s)`: Flush one deflection slot as a single spoken line (" reflektiert N Schüsse").
- L1062 `bool RuleDeflect(const char* text)`: "Reflexionsstatistik: reflektiert Projektil mit N = gegen Angriff N" — one full breakdown per deflected pellet.
- L1108 `bool RuleBedrohung(const char* text)`: Bedrohungsstatistik is the critical-hit *confirmation* roll.
- L1117 `bool RuleKill(const char* text)`: " neutralisiert : N EPs" — a defeat.
- L1134 `bool RuleMineDetect(const char* text)`: Mine-detection feedback enrichment.
- L1183 `void OnUnmatched(const char* /*text*/)`
- L1192 `void RegisterCombatMsgRules()`
- L1213 `void TickCombatAbsorb()`
- L1221 `void TickCombatDeflect()`
- L1231 `void TickCombatEffects()`
