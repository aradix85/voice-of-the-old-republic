// The attack sequence as one spoken line: collecting the summary and
// breakdown lines, then the outgoing or incoming line, and the urgent status
// line.

#include "combat_log_internal.h"

#include "combat_strings.h"
#include "msg_router.h"
#include "party_cache.h"     // IsPartyMember
#include "speech.h"

namespace acc::combat {

namespace {

AttackBlock g_pending = {};

// The whole block on one line, for the log of an attack that is not spoken.
void BuildCompact(const AttackBlock& b, char* out, size_t cap) {
    const auto& L = acc::combat::loc::Get();
    size_t n = 0;
    auto add = [&](const char* fmt, auto... args) {
        if (n + 1 >= cap) return;
        int k = snprintf(out + n, cap - n, fmt, args...);
        if (k > 0) n = (n + k >= cap) ? cap - 1 : n + k;
    };
    add("%s %s", b.actor, b.hit ? L.verb_hit : L.verb_miss);
    if (b.hit && b.crit_tag) add(" %s", L.word_critical);
    add(" %s", b.target);
    if (b.feat[0]) add(" (%s)", b.feat);
    add(". %s %d", L.word_attack, b.sum_attack);
    if (b.has_attack) add(" (%s)", b.attack_comps);
    add(" %s %s %d", L.word_vs, L.word_def, b.sum_defense);
    if (b.has_defense) add(" (%s)", b.defense_nums);
    add(".");
    if (b.has_damage) add(" %s.", b.damage_text);
    else if (b.hit) add(" %s %d.", L.word_damage, b.sum_damage);
    if (b.auto_hit) add(" %s", L.word_auto_hit);
    else if (b.auto_fail) add(" %s", L.word_auto_fail);
}

// ---- Short results-only lines.
//
// The verbose stat breakdown (Attack / Defense / Damage Breakdown) stays in
// the engine message log; only the urgent result is spoken. Actor-led for
// the party's own special moves, victim-led for incoming hits — the
// "who's hurt" name leads when damage lands on us.
//
// Common tail: ", critical" on a crit and ", <status>" when an effect was
// applied. Damage type keeps the engine's casing ("Energy", "Physical",
// "Ion").

void BuildResultTail(const AttackBlock& b, char* tail, size_t cap) {
    const auto& L = acc::combat::loc::Get();
    char crit[24] = "";
    if (b.crit_tag) snprintf(crit, sizeof(crit), ", %s", L.word_critical);
    char status[112] = "";
    if (b.status[0]) snprintf(status, sizeof(status), ", %s", b.status);
    snprintf(tail, cap, "%s%s", crit, status);
}

// The party's own attack. Feats name the move; plain auto-attacks omit it
// (a feat replaces the auto-attack for that swing, so the two never collide):
//   feat hit    → "<actor>, <feat>, <target>, <dmg> <type>[, critical][, status]"
//   feat miss   → "<actor>, failed, <feat>, <target>"
//   plain hit   → "<actor>, <target>, <dmg> <type>[, critical][, status]"
//   plain miss  → (not emitted — caller suppresses)
// The target is named: companions pick their own targets and a fight often
// has several enemies, so "who took it" is not implied by the actor.
void BuildOutgoingLine(const AttackBlock& b, char* out, size_t cap) {
    const auto& L = acc::combat::loc::Get();
    char target_part[110] = "";
    if (b.target[0]) snprintf(target_part, sizeof(target_part), ", %s", b.target);
    if (!b.hit) {
        // Only feats reach a spoken miss; plain misses are suppressed upstream.
        snprintf(out, cap, "%s, %s, %s%s", b.actor, L.word_failed, b.feat, target_part);
        return;
    }
    char tail[140]; BuildResultTail(b, tail, sizeof(tail));
    char feat_part[110] = "";
    if (b.feat[0]) snprintf(feat_part, sizeof(feat_part), ", %s", b.feat);
    if (b.dmg_type[0]) {
        snprintf(out, cap, "%s%s%s, %d %s%s",
                 b.actor, feat_part, target_part, b.sum_damage, b.dmg_type, tail);
    } else {
        snprintf(out, cap, "%s%s%s, %d%s",
                 b.actor, feat_part, target_part, b.sum_damage, tail);
    }
}

// Incoming hit on a party member: "<target>: <dmg> <type> from <actor>[, ...]".
void BuildIncomingLine(const AttackBlock& b, char* out, size_t cap) {
    const auto& L = acc::combat::loc::Get();
    char tail[140]; BuildResultTail(b, tail, sizeof(tail));
    if (b.dmg_type[0]) {
        snprintf(out, cap, "%s: %d %s %s %s%s",
                 b.target, b.sum_damage, b.dmg_type, L.word_from, b.actor, tail);
    } else {
        snprintf(out, cap, "%s: %d %s %s%s",
                 b.target, b.sum_damage, L.word_from, b.actor, tail);
    }
}

// Filter + emit decision for a fully-buffered attack block.
//
//   party attack (feat, or plain hit)  → actor-led result line
//     - feats announce on hit or miss (a deliberate move that whiffs matters)
//     - plain auto-attacks announce on hit only (the damage is what kills;
//       a plain miss isn't urgent)
//   incoming hit (party target)         → victim-led result line
//   anything else                       → suppress speech (still log)
//
// "Party" = active roster of CSWPartyTable (PC + followers). NPC-vs-NPC
// crossfire and the party's plain *misses* stay silent — full detail
// remains in the engine message log.
void FlushPending() {
    if (!g_pending.has_summary) return;
    auto& r = acc::msg::Router::Instance();

    const bool is_feat         = (g_pending.feat[0] != 0);
    const bool actor_is_party  = IsPartyMember(g_pending.actor);
    const bool target_is_party = IsPartyMember(g_pending.target);
    const bool announce_outgoing = actor_is_party && (is_feat || g_pending.hit);

    char buf[640];
    if (announce_outgoing) {
        BuildOutgoingLine(g_pending, buf, sizeof(buf));
        r.Speak(buf);
        r.LogEmit("emit-outgoing", buf);
    } else if (g_pending.hit && target_is_party) {
        // Peel any applied status off the damage line and re-speak it
        // urgently (see MaybeSpeakStatusUrgent). The damage stays normal.
        if (MaybeSpeakStatusUrgent(g_pending.target, g_pending.status))
            g_pending.status[0] = '\0';
        BuildIncomingLine(g_pending, buf, sizeof(buf));
        r.Speak(buf);
        r.LogEmit("emit-incoming", buf);
    } else {
        BuildCompact(g_pending, buf, sizeof(buf));
        r.LogEmit("emit-suppressed", buf);
    }
    g_pending = {};
}

}  // namespace

// A status condition applied to a party member (stunned, poisoned, paralysed,
// feared — whatever the engine echoes in "<target> is <status>") is a
// disabling state the player needs to act on, so it shouldn't wait behind
// queued combat chatter. When `victim` is a party member and `status` is
// non-empty, speak it as its own interrupting line ("<victim> is <status>"),
// like a kill, and return true so the caller drops the status from the
// queued damage/effect line. The status word is moved, not duplicated.
bool MaybeSpeakStatusUrgent(const char* victim, const char* status) {
    if (!status || !status[0] || !IsPartyMember(victim)) return false;
    const auto& L = acc::combat::loc::Get();
    char line[160];
    snprintf(line, sizeof(line), "%s%s%s", victim, L.status_is_marker, status);
    acc::speech::Speak(line, /*interrupt=*/true);
    acc::msg::Router::Instance().LogEmit("emit-status-urgent", line);
    return true;
}

// Router rules. Each returns true to claim the line (suppress raw speech).

bool RuleSummary(const char* text) {
    AttackBlock probe = {};
    if (!ParseSummary(text, probe)) return false;
    if (g_pending.has_summary) FlushPending();
    g_pending = probe;
    return true;
}

bool RuleAttack(const char* text) {
    if (!g_pending.has_summary) return false;
    return ParseAttack(text, g_pending);
}

bool RuleDefense(const char* text) {
    if (!g_pending.has_summary) return false;
    return ParseDefense(text, g_pending);
}

bool RuleDamage(const char* text) {
    if (!g_pending.has_summary) return false;
    if (!ParseDamage(text, g_pending)) return false;
    FlushPending();
    return true;
}

// The Threat Breakdown is the critical-hit *confirmation* roll. Its only
// actionable outcome — did the hit crit — is already carried by the summary's
// "Critical Hit!" tag, which the merged attack line speaks as
// ", critical". The roll math (threat range, confirmation attack vs defense)
// is review-log detail, so claim + suppress it instead of speaking it raw.
bool RuleThreat(const char* text) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_threat)) return false;
    return true;  // claimed, no speech
}

void OnUnmatched(const char* /*text*/) {
    // Unknown mid-block line is a clean boundary — emit whatever
    // breakdown we have (typically a miss whose Damage Breakdown
    // never arrives) before the router speaks the new line raw.
    if (g_pending.has_summary) FlushPending();
}

}  // namespace acc::combat
