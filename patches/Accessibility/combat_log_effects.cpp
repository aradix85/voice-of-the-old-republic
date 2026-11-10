// Abilities, grenades and Force powers: who used what, the saving throws and damage per target, merged into one line per use.

#include "combat_log_internal.h"
#include "msg_router.h"
#include "party_cache.h"     // IsPartyMember

#include "combat_strings.h"

namespace acc::combat {

namespace {

// ---- Ability / grenade / force-power effect merging.
// A power's use, each target's saving throw, the damage, and any applied
// status arrive as separate engine lines (interleaved with other combat
// chatter). We remember the last "<actor> uses <action>" and accumulate per-
// target outcomes into slots, flushed as one merged line on a debounce.
constexpr DWORD kAbilityWindowMs = 5000;   // " uses " stays attributable this long
constexpr DWORD kFxQuietMs       = 500;    // flush a target's effect after this gap
constexpr DWORD kFxMaxHoldMs     = 2500;
constexpr int   kMaxFx           = 12;

struct LastAbility {
    char  actor[96];
    char  action[96];
    DWORD tick;
    bool  valid;
};
LastAbility g_lastAbility = {};

struct EffectTarget {
    bool  used;
    char  target[96];
    char  action[96];   // attributed power/grenade ("" if unknown)
    char  caster[96];   // who used it ("" if unknown)
    bool  has_save;
    bool  saved;
    char  save_type[40];  // "Reflex" / "Fortitude" / "Will"
    bool  has_dmg;
    int   dmg;
    char  dmg_type[32];
    char  status[64];     // applied status, e.g. "stunned"
    DWORD first_tick;
    DWORD last_tick;
};
EffectTarget g_fx[kMaxFx] = {};

EffectTarget* FxFind(const char* target) {
    for (int i = 0; i < kMaxFx; ++i)
        if (g_fx[i].used && strcmp(g_fx[i].target, target) == 0) return &g_fx[i];
    return nullptr;
}

// Find-or-allocate a slot for target. On first allocation, attribute the
// most recent ability use if it's still fresh. Returns nullptr if the table
// is full (caller then leaves the line on the raw-speech fallback).
EffectTarget* FxAlloc(const char* target) {
    EffectTarget* e = FxFind(target);
    if (e) return e;
    for (int i = 0; i < kMaxFx; ++i) {
        if (g_fx[i].used) continue;
        e = &g_fx[i];
        *e = {};
        e->used = true;
        size_t n = strnlen(target, sizeof(e->target) - 1);
        memcpy(e->target, target, n); e->target[n] = '\0';
        DWORD now = GetTickCount();
        e->first_tick = now;
        e->last_tick  = now;
        if (g_lastAbility.valid && (now - g_lastAbility.tick) <= kAbilityWindowMs) {
            memcpy(e->action, g_lastAbility.action, strnlen(g_lastAbility.action, sizeof(e->action) - 1) + 1);
            memcpy(e->caster, g_lastAbility.actor,  strnlen(g_lastAbility.actor,  sizeof(e->caster) - 1) + 1);
        }
        return e;
    }
    return nullptr;
}

// Build one merged per-target effect line. Empty `out` = nothing to say.
void BuildEffectLine(const EffectTarget& e, char* out, size_t cap) {
    const auto& L = acc::combat::loc::Get();
    out[0] = '\0';

    char dmg_clause[48] = "";
    if (e.has_dmg && e.dmg > 0) {
        if (e.dmg_type[0]) snprintf(dmg_clause, sizeof(dmg_clause), "%d %s", e.dmg, e.dmg_type);
        else               snprintf(dmg_clause, sizeof(dmg_clause), "%d", e.dmg);
    }
    char from_action[120] = "";   // " from <action>"
    if (e.action[0]) snprintf(from_action, sizeof(from_action), " %s %s", L.word_from, e.action);
    char action_only[110] = "";  // " <action>"
    if (e.action[0]) snprintf(action_only, sizeof(action_only), " %s", e.action);

    if (e.has_save && e.saved) {
        // Resisted: "<target> resists <action>[, <dmg>], <SaveType>"
        if (dmg_clause[0])
            snprintf(out, cap, "%s %s%s, %s, %s",
                     e.target, L.word_resists, action_only, dmg_clause, e.save_type);
        else
            snprintf(out, cap, "%s %s%s, %s",
                     e.target, L.word_resists, action_only, e.save_type);
    } else if (e.has_save && !e.saved) {
        // Failed save: damage and/or status, tagged "<SaveType> failed".
        if (dmg_clause[0] && e.status[0])
            snprintf(out, cap, "%s: %s%s, %s, %s %s",
                     e.target, dmg_clause, from_action, e.status, e.save_type, L.word_save_failed);
        else if (dmg_clause[0])
            snprintf(out, cap, "%s: %s%s, %s %s",
                     e.target, dmg_clause, from_action, e.save_type, L.word_save_failed);
        else if (e.status[0])
            snprintf(out, cap, "%s %s%s, %s %s",
                     e.target, e.status, from_action, e.save_type, L.word_save_failed);
        else
            snprintf(out, cap, "%s%s, %s %s",
                     e.target, action_only, e.save_type, L.word_save_failed);
    } else {
        // No save (direct damage / status): "<target>: <dmg> from <action>[, <status>]"
        char status_clause[80] = "";
        if (e.status[0]) snprintf(status_clause, sizeof(status_clause), ", %s", e.status);
        if (dmg_clause[0])
            snprintf(out, cap, "%s: %s%s%s", e.target, dmg_clause, from_action, status_clause);
        else if (e.status[0])
            snprintf(out, cap, "%s%s%s", e.target, action_only, status_clause);
        // else: a bare save-less, damage-less, status-less line — nothing to say.
    }
}

// Speak (and clear) one accumulated effect slot. Party-relevant only — an
// NPC-vs-NPC power exchange is logged but not spoken, matching the weapon path.
void FlushEffect(EffectTarget& e) {
    // A force/grenade status on a party member: speak it on its own first,
    // before building the line, so the disabling state cuts through (same as
    // the weapon path in FlushPending). Status moved, not duplicated.
    if (MaybeSpeakStatusUrgent(e.target, e.status)) e.status[0] = '\0';
    char line[300];
    BuildEffectLine(e, line, sizeof(line));
    if (line[0]) {
        auto& r = acc::msg::Router::Instance();
        // Suppress ONLY a positively-identified NPC-vs-NPC effect: a known
        // non-party caster acting on a non-party target. An unknown caster
        // (powers that emit no " uses " line — e.g. a Stun queued during
        // pause) defaults to announce: in the player's own fight an
        // unattributed effect is almost always something we caused.
        bool caster_is_party = (e.caster[0] && IsPartyMember(e.caster));
        bool target_is_party = IsPartyMember(e.target);
        bool known_npc_vs_npc =
            (e.caster[0] && !caster_is_party && !target_is_party);
        if (!known_npc_vs_npc) {
            acc::msg::Router::Instance().Speak(line);
            r.LogEmit("emit-effect", line);
        }
        else                   { r.LogEmit("emit-effect-suppressed", line); }
    }
    e = {};  // clears used
}

}  // namespace

// "Effect Application Breakdown:<target> is <status>" — the engine's trailing status
// echo. Two cases:
//   * A force power applied it: an effect slot for <target> already exists —
//     attach the status so the merged effect line speaks it.
//   * A weapon attack applied it: no slot exists; the status was already
//     folded into the attack line from the summary tail — drop the duplicate.
// Either way we claim the line (no raw speech).
bool RuleEffectApplication(const char* text) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_effect_application)) return false;
    const char* p   = text + strlen(L.prefix_effect_application);
    const char* is_at = strstr(p, L.status_is_marker);
    if (is_at) {
        char target[96];
        CopyRange(target, sizeof(target), p, is_at);
        EffectTarget* e = FxFind(target);
        if (e) {
            const char* s = is_at + strlen(L.status_is_marker);
            CopyRange(e->status, sizeof(e->status), s, s + strlen(s));
            e->last_tick = GetTickCount();
        }
    }
    return true;  // claimed, no speech
}

// "<actor> uses <action>[. ...]" — a power/grenade
// /item use. Record it for effect attribution, and speak a shortened cue
// (dropping the Force point math) only for the party's own casts/throws —
// incoming enemy powers announce via their effect line instead.
bool RuleAbilityUse(const char* text) {
    const auto& L = acc::combat::loc::Get();
    const char* m = strstr(text, L.ability_use_marker);   // " uses "
    if (!m) return false;
    char actor[96];
    CopyRange(actor, sizeof(actor), text, m);
    const char* as  = m + strlen(L.ability_use_marker);
    const char* dot = strchr(as, '.');
    const char* ae  = dot ? dot : as + strlen(as);
    char action[96];
    CopyRange(action, sizeof(action), as, ae);
    if (!actor[0] || !action[0]) return false;

    DWORD now = GetTickCount();
    memcpy(g_lastAbility.actor,  actor,  strnlen(actor,  sizeof(g_lastAbility.actor)  - 1) + 1);
    memcpy(g_lastAbility.action, action, strnlen(action, sizeof(g_lastAbility.action) - 1) + 1);
    g_lastAbility.tick  = now;
    g_lastAbility.valid = true;

    auto& r = acc::msg::Router::Instance();
    if (IsPartyMember(actor)) {
        char line[200];
        snprintf(line, sizeof(line), "%s%s%s", actor, L.ability_use_marker, action);
        acc::msg::Router::Instance().Speak(line);
        r.LogEmit("emit-use", line);
    } else {
        r.LogEmit("emit-use-suppressed", action);
    }
    return true;  // claimed (suppress the verbose raw use line)
}

// "<saver> <SaveType> Save. success!|failure! ..." — buffer the
// outcome into the saver's effect slot; flushed merged on the debounce.
bool RuleSaveThrow(const char* text) {
    const auto& L = acc::combat::loc::Get();
    const char* sm = strstr(text, L.save_marker);   // " Save. "
    if (!sm) return false;
    const char* ts = sm;
    while (ts > text && ts[-1] != ' ') --ts;        // start of the SaveType word
    char save_type[40];
    CopyRange(save_type, sizeof(save_type), ts, sm);
    char saver[96];
    CopyRange(saver, sizeof(saver), text, ts);
    if (!saver[0]) return false;

    bool saved;
    if (strstr(sm, L.save_success))   saved = true;
    else if (strstr(sm, L.save_fail)) saved = false;
    else return false;

    EffectTarget* e = FxAlloc(saver);
    if (!e) return false;  // table full — let it speak raw
    e->has_save = true;
    e->saved    = saved;
    memcpy(e->save_type, save_type, strnlen(save_type, sizeof(e->save_type) - 1) + 1);
    e->last_tick = GetTickCount();
    return true;
}

// "<actor> damages <target>: N damage (Type: N)" — direct/ability damage.
bool RuleDirectDamage(const char* text) {
    const auto& L = acc::combat::loc::Get();
    const char* dm = strstr(text, L.damage_marker);  // " damages "
    if (!dm) return false;
    char actor[96];
    CopyRange(actor, sizeof(actor), text, dm);
    const char* tstart = dm + strlen(L.damage_marker);
    const char* tend = strchr(tstart, ':');
    if (!tend) return false;
    const char* amount = tend + 1;
    char target[96];
    CopyRange(target, sizeof(target), tstart, tend);
    if (!actor[0] || !target[0]) return false;

    const char* p = amount;
    while (*p == ' ') ++p;
    int dmg = atoi(p);

    char dtype[32] = "";
    const char* paren = strchr(p, '(');
    if (paren) {
        const char* tt = paren + 1;
        const char* tc = strchr(tt, ':');
        if (tc) CopyRange(dtype, sizeof(dtype), tt, tc);
    }

    EffectTarget* e = FxAlloc(target);
    if (!e) return false;
    e->has_dmg = true;
    e->dmg     = dmg;
    if (dtype[0]) memcpy(e->dmg_type, dtype, strnlen(dtype, sizeof(e->dmg_type) - 1) + 1);
    if (!e->caster[0]) memcpy(e->caster, actor, strnlen(actor, sizeof(e->caster) - 1) + 1);
    e->last_tick = GetTickCount();
    return true;
}

void TickCombatEffects() {
    DWORD now = GetTickCount();
    for (int i = 0; i < kMaxFx; ++i) {
        if (!g_fx[i].used) continue;
        bool quiet  = (now - g_fx[i].last_tick)  >= kFxQuietMs;
        bool maxAge = (now - g_fx[i].first_tick) >= kFxMaxHoldMs;
        if (quiet || maxAge) FlushEffect(g_fx[i]);
    }
}

}  // namespace acc::combat
