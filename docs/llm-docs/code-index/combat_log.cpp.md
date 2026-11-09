# combat_log.cpp (1300 lines)

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
- L213 `void ParseSummaryTail(const char* text, const char* mit_at, AttackBlock& b)`: Numeric tail of the attack summary, shared by both layouts: the attack / defence / damage values behind phrase_mit, and the crit and auto tags.
- L233 `bool ParseSummaryLabelled(const char* text, AttackBlock& b)`: Labelled summary layout — Polish.
- L277 `bool ParseSummaryClassic(const char* text, AttackBlock& b)`: Classic summary layout — DE/EN/FR/IT/ES/RU.
- L308 `bool ParseSummary(const char* text, AttackBlock& b)`
- L338 `bool ParseAngriff(const char* text, AttackBlock& b)`
- L439 `bool ParseAbwehr(const char* text, AttackBlock& b)`
- L477 `bool ParseSchaden(const char* text, AttackBlock& b)`
- L603 `void BuildCompact(const AttackBlock& b, char* out, size_t cap)`
- L675 `void BuildResultTail(const AttackBlock& b, char* tail, size_t cap)`
- L694 `void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap)`: The party's own attack.
- L716 `void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap)`: Incoming hit on a party member: " : von [, ...]".
- L735 `bool MaybeSpeakStatusUrgent(const char* victim, const char* status)`: A status condition applied to a party member (stunned, poisoned, paralysed, feared — whatever the engine echoes in " ist ") is a disabling state the player needs to act on, so it shouldn't wait behind queued combat ch...
- L757 `void FlushPending()`: Filter + emit decision for a fully-buffered attack block.
- L788 `bool RuleSummary(const char* text)`
- L796 `bool RuleAngriff(const char* text)`
- L801 `bool RuleAbwehr(const char* text)`
- L806 `bool RuleSchaden(const char* text)`
- L820 `bool RuleAuswirkung(const char* text)`: "Auswirkungsstatistik: ist " — the engine's trailing status echo.
- L842 `bool RuleAbilityUse(const char* text)`: " benutzt [.
- L875 `bool RuleSaveThrow(const char* text)`: " -Rettungswurf.
- L902 `bool RuleDirectDamage(const char* text)`: " verletzt : N Schaden (Type: N)" — direct/ability damage.
- L949 `void BuildEffectLine(const EffectTarget& e, char* out, size_t cap)`: Build one merged per-target effect line.
- L999 `void FlushEffect(EffectTarget& e)`: Speak (and clear) one accumulated effect slot.
- L1028 `void FlushAbsorb()`: Flush the pending absorb burst as one spoken total.
- L1045 `bool RuleAbsorb(const char* text)`: Accumulate a damage-absorption line into the pending burst (claimed — no raw speech).
- L1098 `void FlushDeflect(DeflectSlot& s)`: Flush one deflection slot as a single spoken line (" reflektiert N Schüsse").
- L1120 `bool RuleDeflect(const char* text)`: "Reflexionsstatistik: reflektiert Projektil mit N = gegen Angriff N" — one full breakdown per deflected pellet.
- L1166 `bool RuleBedrohung(const char* text)`: Bedrohungsstatistik is the critical-hit *confirmation* roll.
- L1175 `bool RuleKill(const char* text)`: " neutralisiert : N EPs" — a defeat.
- L1192 `bool RuleMineDetect(const char* text)`: Mine-detection feedback enrichment.
- L1241 `void OnUnmatched(const char* /*text*/)`
- L1250 `void RegisterCombatMsgRules()`
- L1271 `void TickCombatAbsorb()`
- L1279 `void TickCombatDeflect()`
- L1289 `void TickCombatEffects()`
