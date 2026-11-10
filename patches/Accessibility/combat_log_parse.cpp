// Parsing the engine's attack sequence: the summary line and the attack,
// defense and damage breakdowns, into one AttackBlock.

#include "combat_log_internal.h"

#include "combat_strings.h"

namespace acc::combat {

namespace {

// Numeric tail of the attack summary: the attack / defence / damage values
// behind phrase_with, and the crit and auto tags. `with_at` is phrase_with's
// position; the head parse finds it first, since it bounds the target.
void ParseSummaryTail(const char* text, const char* with_at, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    const char* p = with_at + strlen(L.phrase_with);
    b.sum_attack = atoi(p);

    const char* def_at = strstr(p, L.word_defense);
    if (def_at) b.sum_defense = atoi(def_at + strlen(L.word_defense));

    const char* damage_at = strstr(p, L.word_damage_colon);
    if (damage_at) b.sum_damage = atoi(damage_at + strlen(L.word_damage_colon));

    if (strstr(text, L.tag_crit_summary)) b.crit_tag  = true;
    if (strstr(text, L.tag_auto_hit))     b.auto_hit  = true;
    if (strstr(text, L.tag_auto_fail))    b.auto_fail = true;
}

// Head of the attack summary: "<actor><phrase><target>. [<feat> used.]
// <phrase_with>...", where the phrase between actor and target is itself the
// hit/miss signal.
bool ParseSummaryHead(const char* text, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    const char* hit_at  = strstr(text, L.phrase_hit);
    const char* miss_at = strstr(text, L.phrase_miss);
    if (!hit_at && !miss_at) return false;
    const char* anchor = hit_at ? hit_at : miss_at;
    size_t alen = strlen(hit_at ? L.phrase_hit : L.phrase_miss);
    b.hit = (hit_at != nullptr);
    CopyRange(b.actor, sizeof(b.actor), text, anchor);

    const char* after_on = anchor + alen;
    const char* with_at = strstr(after_on, L.phrase_with);
    if (!with_at) return false;

    const char* target_dot = strchr(after_on, '.');
    if (!target_dot || target_dot > with_at) return false;
    CopyRange(b.target, sizeof(b.target), after_on, target_dot);

    const char* feat_start = target_dot + 1;
    while (feat_start < with_at && *feat_start == ' ') ++feat_start;
    if (feat_start < with_at) {
        const char* feat_end = strstr(feat_start, L.feat_marker);
        if (feat_end && feat_end < with_at) {
            CopyRange(b.feat, sizeof(b.feat), feat_start, feat_end);
        }
    }

    ParseSummaryTail(text, with_at, b);
    return true;
}

}  // namespace

bool ParseSummary(const char* text, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    b = {};
    if (!ParseSummaryHead(text, b)) return false;

    // Status tail: an applied effect rides the end of the summary as
    // "<target> is <status>" (e.g. "Kath Hound is stunned"). The target
    // name only re-appears here in that clause — "attack on <target>."
    // is followed by '.', not " is " — so a plain search for "<target> is "
    // pins the status (the opening "<actor> succeeds with attack on" has no
    // " is "). Captures everything after to end-of-line.
    if (b.target[0]) {
        char needle[112];
        int nn = snprintf(needle, sizeof(needle), "%s%s", b.target, L.status_is_marker);
        if (nn > 0 && nn < (int)sizeof(needle)) {
            const char* st = strstr(text, needle);
            if (st) {
                const char* s = st + nn;
                const char* e = s + strlen(s);
                CopyRange(b.status, sizeof(b.status), s, e);
            }
        }
    }

    b.has_summary = true;
    return true;
}

bool ParseAttack(const char* text, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_attack)) return false;
    const char* p = text + strlen(L.prefix_attack);

    const char* hand_end = p;
    while (*hand_end && *hand_end != ' ') ++hand_end;
    CopyRange(b.attack_hand, sizeof(b.attack_hand), p, hand_end);
    p = hand_end;
    while (*p == ' ') ++p;

    if (*p == '-') ++p;
    while (*p >= '0' && *p <= '9') ++p;
    while (*p == ' ') ++p;
    if (*p != '=') return false;
    ++p;
    while (*p == ' ') ++p;

    struct Map { const char* from; size_t flen; const char* to; size_t tlen; };
    const Map kMap[] = {
        { L.token_roll,    strlen(L.token_roll),    L.short_roll,   strlen(L.short_roll) },
        { L.token_dex_mod, strlen(L.token_dex_mod), L.short_dex,    strlen(L.short_dex) },
        { L.token_range,   strlen(L.token_range),   L.short_range,  strlen(L.short_range) },
        { L.token_effect,  strlen(L.token_effect),  L.short_effect, strlen(L.short_effect) },
    };

    char out[256];
    size_t out_len = 0;
    bool first = true;

    while (*p) {
        const char* comp_end = strstr(p, "+ ");
        if (!comp_end) comp_end = p + strlen(p);

        // Cut the roll component before any auto-hit/auto-fail tail (the
        // engine glues "roll 1 Automatic Miss!" with no "+" separator). Back
        // up one to include the preceding space.
        const char* auto_h = strstr(p, L.tag_auto_hit);
        const char* auto_f = strstr(p, L.tag_auto_fail);
        const char* auto_at = nullptr;
        if (auto_h && auto_f)      auto_at = (auto_h < auto_f) ? auto_h : auto_f;
        else if (auto_h)           auto_at = auto_h;
        else if (auto_f)           auto_at = auto_f;
        if (auto_at && auto_at > p && auto_at[-1] == ' ') --auto_at;
        if (auto_at && auto_at < comp_end) comp_end = auto_at;

        if (!first && out_len < sizeof(out) - 1) {
            out[out_len++] = '+';
        }
        first = false;

        const Map* m_hit = nullptr;
        for (const auto& m : kMap) {
            if ((size_t)(comp_end - p) >= m.flen &&
                memcmp(p, m.from, m.flen) == 0) {
                m_hit = &m;
                break;
            }
        }
        if (m_hit) {
            size_t copy = m_hit->tlen;
            if (out_len + copy < sizeof(out)) {
                memcpy(out + out_len, m_hit->to, copy);
                out_len += copy;
            }
            const char* tail = p + m_hit->flen;
            copy = (size_t)(comp_end - tail);
            if (out_len + copy >= sizeof(out)) copy = sizeof(out) - 1 - out_len;
            memcpy(out + out_len, tail, copy);
            out_len += copy;
        } else {
            size_t copy = (size_t)(comp_end - p);
            if (out_len + copy >= sizeof(out)) copy = sizeof(out) - 1 - out_len;
            memcpy(out + out_len, p, copy);
            out_len += copy;
        }

        if (auto_at && auto_at == comp_end) break;
        if (!*comp_end) break;
        p = comp_end + 2;
    }
    out[out_len] = '\0';

    // The hand label only when it isn't the main hand.
    if (b.attack_hand[0] && strcmp(b.attack_hand, L.hand_main) != 0) {
        snprintf(b.attack_comps, sizeof(b.attack_comps), "%s %s", b.attack_hand, out);
    } else {
        snprintf(b.attack_comps, sizeof(b.attack_comps), "%s", out);
    }
    b.has_attack = true;
    return true;
}

bool ParseDefense(const char* text, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_defense)) return false;
    const char* p = text + strlen(L.prefix_defense);
    const char* eq = strstr(p, " = ");
    if (!eq) return false;
    p = eq + 3;

    char out[64];
    size_t out_len = 0;
    bool first = true;

    while (*p) {
        const char* comp_end = strstr(p, "+ ");
        if (!comp_end) comp_end = p + strlen(p);

        // Number is whatever follows the last space inside the component.
        const char* num_start = comp_end;
        while (num_start > p && *(num_start - 1) != ' ') --num_start;

        if (!first && num_start[0] != '-' && out_len < sizeof(out) - 1) {
            out[out_len++] = '+';
        }
        first = false;
        size_t n = (size_t)(comp_end - num_start);
        if (out_len + n >= sizeof(out)) n = sizeof(out) - 1 - out_len;
        memcpy(out + out_len, num_start, n);
        out_len += n;

        if (!*comp_end) break;
        p = comp_end + 2;
    }
    out[out_len] = '\0';
    memcpy(b.defense_nums, out, out_len + 1);
    b.has_defense = true;
    return true;
}

bool ParseDamage(const char* text, AttackBlock& b) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_damage)) return false;
    const char* p = text + strlen(L.prefix_damage);

    int mult = 0;
    if (MsgStartsWith(p, L.crit_x_prefix)) {
        p += strlen(L.crit_x_prefix);
        mult = atoi(p);
        const char* for_at = strstr(p, L.phrase_for);
        if (for_at) p = for_at + strlen(L.phrase_for);
    }

    const char* eq = strstr(p, " = ");
    if (!eq) return false;
    p = eq + 3;

    // Two parallel outputs:
    //   out      — full-breakdown format "Energy 6+Bonus 5"   (type first, +-joined)
    //   shortBuf — short-form format     "6 Energy + 5 Bonus" (value first, " + "-joined)
    char out[128];      size_t out_len = 0;
    char shortBuf[128]; size_t short_len = 0;
    bool first = true;

    auto EmitTo = [](char* dst, size_t cap, size_t& dst_len, const char* s, size_t n) {
        if (dst_len + n >= cap) n = cap - 1 - dst_len;
        memcpy(dst + dst_len, s, n);
        dst_len += n;
    };
    auto Emit      = [&](const char* s, size_t n) { EmitTo(out,      sizeof(out),      out_len,   s, n); };
    auto EmitShort = [&](const char* s, size_t n) { EmitTo(shortBuf, sizeof(shortBuf), short_len, s, n); };

    while (*p) {
        const char* comp_end = strstr(p, " + ");
        if (!comp_end) comp_end = p + strlen(p);

        // The engine pads some separators to a double space
        // ("universal 1  + strength mod 5"), leaving the component with a
        // trailing space. Trim it, or the last-space split below finds an
        // empty value and the number leaks into the type ("6 universal 1").
        const char* text_end = comp_end;
        while (text_end > p && text_end[-1] == ' ') --text_end;

        if (!first) { Emit("+", 1); EmitShort(" + ", 3); }
        first = false;

        // Resolve a (type-label, value-string) pair for this component.
        // Either "Energy: 6" (colon split) or "bonus damage 5" (no colon,
        // last-space split with label shortening).
        const char* type_start = p;
        const char* type_end   = nullptr;
        const char* val_start  = nullptr;
        const char* val_end    = text_end;

        const char* colon = nullptr;
        for (const char* q = p; q + 1 < text_end; ++q) {
            if (q[0] == ':' && q[1] == ' ') { colon = q; break; }
        }

        // Per-component label shortening replacement for the type label;
        // applied AFTER we determine where the type substring sits.
        const char* type_replacement     = nullptr;
        size_t      type_replacement_len = 0;

        if (colon) {
            type_end  = colon;
            val_start = colon + 2;
        } else {
            // No colon — "bonus damage" style. Number is whatever follows the
            // last space.
            const char* space = text_end;
            while (space > p && *(space - 1) != ' ') --space;
            type_end = (space > p) ? (space - 1) : p;
            val_start = space;

            const size_t kBonusFromLen = strlen(L.token_bonus_damage);
            if ((size_t)(type_end - type_start) == kBonusFromLen &&
                memcmp(type_start, L.token_bonus_damage, kBonusFromLen) == 0) {
                type_replacement     = L.short_bonus;
                type_replacement_len = strlen(L.short_bonus);
            }
        }

        // Primary damage type = first component's type label (the elemental
        // type: Energy / Physical / Ion / ...). Bonus components
        // (special weapon, Strength mod., bonus damage) are additive, not types,
        // and always follow — so the first label is the one to speak.
        if (b.dmg_type[0] == '\0' && !type_replacement && type_end > type_start) {
            CopyRange(b.dmg_type, sizeof(b.dmg_type), type_start, type_end);
        }

        // Full-breakdown form: "<Type> <value>"
        if (type_replacement) {
            Emit(type_replacement, type_replacement_len);
        } else {
            Emit(type_start, (size_t)(type_end - type_start));
        }
        Emit(" ", 1);
        Emit(val_start, (size_t)(val_end - val_start));

        // Short-form: "<value> <Type>"
        EmitShort(val_start, (size_t)(val_end - val_start));
        EmitShort(" ", 1);
        if (type_replacement) {
            EmitShort(type_replacement, type_replacement_len);
        } else {
            EmitShort(type_start, (size_t)(type_end - type_start));
        }

        if (!*comp_end) break;
        p = comp_end + 3;  // skip " + "
    }

    if (mult > 0) {
        char tail[16];
        int n = snprintf(tail, sizeof(tail), " x%d", mult);
        if (n > 0) Emit(tail, (size_t)n);
    }
    out[out_len]            = '\0';
    shortBuf[short_len]     = '\0';
    memcpy(b.damage_text,  out,      out_len   + 1);
    memcpy(b.damage_short, shortBuf, short_len + 1);
    b.has_damage = true;
    return true;
}

}  // namespace acc::combat
