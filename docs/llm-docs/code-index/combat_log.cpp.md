# combat_log.cpp (1333 lines)

Source: `patches/Accessibility/combat_log.cpp`

combat-log message parsing and the spoken result lines.

Split out of combat.cpp by the Phase-1 structure pass (refactoring candidate 7). combat.cpp keeps combat-mode tracking (TickCombatMode / TickCombatLog and the IsCombatActive / IsPartyInCombat predicates); everything here is the acc::msg::Router rule set that turns the engine's localized combat-log lines into the mod's short spoken announcements.

The two halves already lived in separate namespace blocks in the original file and share no state - the parser owns all of its own statics, and core_tick reaches the three tickers through combat.h. The move is verbatim, including this file's own extra include block below.

Parsing is anchor-based against the localized strings in combat_strings.*; see project_combat_loc_en_phrase_mit_bug for why the anchors are fragile and must not be "tidied".

## Declarations (in source order)

- L165 `EffectTarget* FxFind(const char* target)`
- L174 `EffectTarget* FxAlloc(const char* target)`: Find-or-allocate a slot for target.
- L196 `bool MsgStartsWith(const char* s, const char* p)`
- L201 `void CopyRange(char* dst, size_t cap, const char* start, const char* end)`
- L214 `void ParseSummaryTail(const char* text, const char* mit_at, AttackBlock& b)`: Numeric tail of the attack summary, shared by both layouts: the attack / defence / damage values behind phrase_mit, and the crit and auto tags.
- L234 `bool ParseSummaryLabelled(const char* text, AttackBlock& b)`: Labelled summary layout — Polish.
- L278 `bool ParseSummaryClassic(const char* text, AttackBlock& b)`: Classic summary layout — DE/EN/FR/IT/ES/RU.
- L309 `bool ParseSummary(const char* text, AttackBlock& b)`
- L339 `bool ParseAngriff(const char* text, AttackBlock& b)`
- L440 `bool ParseAbwehr(const char* text, AttackBlock& b)`
- L478 `bool ParseSchaden(const char* text, AttackBlock& b)`
- L604 `void BuildCompact(const AttackBlock& b, char* out, size_t cap)`
- L676 `void BuildResultTail(const AttackBlock& b, char* tail, size_t cap)`
- L695 `void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap)`: The party's own attack.
- L717 `void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap)`: Incoming hit on a party member: " : von [, ...]".
- L736 `void SpeakCombatLine(const char* line, const char* subject, const char* nearName = nullptr)`: Every spoken combat line goes through one of these two.
- L742 `void SpeakCombatLineUrgent(const char* line, const char* subject)`
- L757 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: A status condition applied to a party member (stunned, poisoned, paralysed, feared — whatever the engine echoes in " ist ") is a disabling state the player needs to act on, so it shouldn't wait behind queued combat ch...
- L779 `void FlushPending()`: Filter + emit decision for a fully-buffered attack block.
- L810 `bool RuleSummary(const char* text)`
- L818 `bool RuleAngriff(const char* text)`
- L823 `bool RuleAbwehr(const char* text)`
- L828 `bool RuleSchaden(const char* text)`
- L842 `bool RuleAuswirkung(const char* text)`: "Auswirkungsstatistik: ist " — the engine's trailing status echo.
- L864 `bool RuleAbilityUse(const char* text)`: " benutzt [.
- L897 `bool RuleSaveThrow(const char* text)`: " -Rettungswurf.
- L924 `bool RuleDirectDamage(const char* text)`: " verletzt : N Schaden (Type: N)" — direct/ability damage.
- L971 `void BuildEffectLine(const EffectTarget& e, char* out, size_t cap)`: Build one merged per-target effect line.
- L1021 `void FlushEffect(EffectTarget& e)`: Speak (and clear) one accumulated effect slot.
- L1050 `void FlushAbsorb()`: Flush the pending absorb burst as one spoken total.
- L1067 `bool RuleAbsorb(const char* text)`: Accumulate a damage-absorption line into the pending burst (claimed — no raw speech).
- L1120 `void FlushDeflect(DeflectSlot& s)`: Flush one deflection slot as a single spoken line (" reflektiert N Schüsse").
- L1142 `bool RuleDeflect(const char* text)`: "Reflexionsstatistik: reflektiert Projektil mit N = gegen Angriff N" — one full breakdown per deflected pellet.
- L1188 `bool RuleBedrohung(const char* text)`: Bedrohungsstatistik is the critical-hit *confirmation* roll.
- L1202 `bool RuleKill(const char* text)`: " neutralisiert : N EPs" — a defeat.
- L1225 `bool RuleMineDetect(const char* text)`: Mine-detection feedback enrichment.
- L1274 `void OnUnmatched(const char* /*text*/)`
- L1283 `void RegisterCombatMsgRules()`
- L1304 `void TickCombatAbsorb()`
- L1312 `void TickCombatDeflect()`
- L1322 `void TickCombatEffects()`
