// Energy shields and blaster deflection: absorbed damage and deflected shots, collected and spoken once per burst.

#include "combat_log_internal.h"
#include "msg_router.h"
#include "party_cache.h"     // IsPartyMember

#include "combat_strings.h"

namespace acc::combat {

namespace {

// ---- Damage-absorption burst coalescing.
// One engine line fires per blocked hit; a sustained shield emits dozens.
// We accumulate a burst (running total + count) and flush a single spoken
// line once kAbsorbQuietMs passes with no new absorb. namepart/suffix are
// captured verbatim from the engine line so the spoken total stays locale-
// agnostic: "<namepart>absorbs <total><suffix>".
// Windows widened 600/2500 → 1500/5000 (2026-07-17, Star Forge feedback):
// autofire volleys spaced ~1s kept splitting into a spoken line per volley —
// 296 absorb lines in one session. One line per exchange is the target.
constexpr DWORD kAbsorbQuietMs   = 1500;   // flush after this long with no new absorb
constexpr DWORD kAbsorbMaxHoldMs = 5000;   // ...or this long since the burst began
char  g_absorb_namepart[160] = {};   // text before the anchor (incl. trailing space)
char  g_absorb_suffix[64]    = {};   // text after the number, up to " :" or end
int   g_absorb_total         = 0;
int   g_absorb_count         = 0;
DWORD g_absorb_last_tick     = 0;
DWORD g_absorb_first_tick    = 0;

// ---- Blaster-deflection burst coalescing.
// The engine emits one full "Reflexionsstatistik: ..." breakdown per
// deflected pellet — a Jedi tanking turret autofire produced 322 raw
// spoken lines in one session (patch-20260716-221327.log). We claim the
// line, count per deflector, and flush "<actor> deflects N shots"
// on the same debounce shape as the absorb burst. Party deflectors only
// reach speech; NPC-vs-NPC deflections are logged and dropped, matching
// the weapon-path policy.
constexpr DWORD kDeflectQuietMs   = 1500;
constexpr DWORD kDeflectMaxHoldMs = 5000;
constexpr int   kMaxDeflectSlots  = 4;   // concurrent distinct deflectors

struct DeflectSlot {
    bool  used;
    char  actor[96];
    int   count;
    DWORD first_tick;
    DWORD last_tick;
};
DeflectSlot g_deflect[kMaxDeflectSlots] = {};

// Flush the pending absorb burst as one spoken total. Reconstructed from the
// captured namepart + engine anchor + accumulated total + suffix.
void FlushAbsorb() {
    if (g_absorb_count == 0) return;
    const auto& L = acc::combat::loc::Get();
    char line[256];
    snprintf(line, sizeof(line), "%s%s %d%s",
             g_absorb_namepart, L.absorb_anchor, g_absorb_total, g_absorb_suffix);
    auto& r = acc::msg::Router::Instance();
    acc::msg::Router::Instance().Speak(line);
    r.LogEmit("emit-absorb", line);
    g_absorb_namepart[0] = '\0';
    g_absorb_suffix[0]   = '\0';
    g_absorb_total = 0;
    g_absorb_count = 0;
}

// Flush one deflection slot as a single spoken line ("<actor> deflects
// N shots"). Party deflectors only; an NPC deflecting our shots is the
// same information the suppressed miss line already carried, so it stays
// log-only.
void FlushDeflect(DeflectSlot& s) {
    if (!s.used || s.count <= 0) { s = {}; return; }
    const auto& L = acc::combat::loc::Get();
    char line[160];
    if (s.count == 1) snprintf(line, sizeof(line), L.fmt_deflect_one, s.actor);
    else              snprintf(line, sizeof(line), L.fmt_deflect_many, s.actor, s.count);
    auto& r = acc::msg::Router::Instance();
    if (IsPartyMember(s.actor)) {
        acc::msg::Router::Instance().Speak(line);
        r.LogEmit("emit-deflect", line);
    } else {
        r.LogEmit("emit-deflect-suppressed", line);
    }
    s = {};
}

}  // namespace

// Accumulate a damage-absorption line into the pending burst (claimed — no
// raw speech). TickCombatAbsorb flushes it on the debounce.
bool RuleAbsorb(const char* text) {
    const auto& L = acc::combat::loc::Get();
    const char* a = strstr(text, L.absorb_anchor);
    if (!a) return false;
    const char* p = a + strlen(L.absorb_anchor);
    while (*p == ' ') ++p;
    if (*p < '0' || *p > '9') return false;   // unexpected shape — let it speak raw
    int pts = atoi(p);
    const char* numEnd = p;
    while (*numEnd >= '0' && *numEnd <= '9') ++numEnd;

    char namepart[160];
    size_t nlen = (size_t)(a - text);
    if (nlen >= sizeof(namepart)) nlen = sizeof(namepart) - 1;
    memcpy(namepart, text, nlen);
    namepart[nlen] = '\0';

    // Normalize the damage-resistance variant ("<name> : Damage Resistance
    // absorbs ...") to the bare "<name> " the energy-shield lines use:
    // cut at the " : " separator, keeping the name's trailing space. Both
    // sources then share one burst key — they fire together for the same
    // volley — and the engine's odd colon phrasing never reaches speech.
    if (char* sep = strstr(namepart, " : ")) {
        sep[1] = '\0';
        nlen = strlen(namepart);
    }

    char suffix[64];
    const char* cut = strstr(numEnd, " :");        // drop "...: M points remaining"
    const char* e   = cut ? cut : numEnd + strlen(numEnd);
    size_t slen = (size_t)(e - numEnd);
    if (slen >= sizeof(suffix)) slen = sizeof(suffix) - 1;
    memcpy(suffix, numEnd, slen);
    suffix[slen] = '\0';

    // A different absorber than the pending burst → flush the old one first.
    if (g_absorb_count > 0 && strcmp(namepart, g_absorb_namepart) != 0) {
        FlushAbsorb();
    }
    DWORD now = GetTickCount();
    if (g_absorb_count == 0) g_absorb_first_tick = now;
    memcpy(g_absorb_namepart, namepart, nlen + 1);
    memcpy(g_absorb_suffix,   suffix,   slen + 1);
    g_absorb_total += pts;
    ++g_absorb_count;
    g_absorb_last_tick = now;
    return true;  // claimed
}

// "Deflection Breakdown: <actor> deflects projectile with N = <roll math>
// vs. attack N" — one full breakdown per deflected pellet. Claim it,
// count per actor, flush via TickCombatDeflect. The matching miss summary
// ("... Deflected!") is already suppressed with all plain misses, so this
// burst line is the only audible trace of deflection — by design one line
// per volley instead of one per pellet.
bool RuleDeflect(const char* text) {
    const auto& L = acc::combat::loc::Get();
    if (!MsgStartsWith(text, L.prefix_deflection)) return false;

    char actor[96] = "";
    const char* astart = text + strlen(L.prefix_deflection);
    const char* mid    = strstr(astart, L.reflect_mid_marker);
    if (mid) CopyRange(actor, sizeof(actor), astart, mid);
    if (!actor[0]) return true;  // unexpected shape — still claim; the
                                 // breakdown must never reach raw speech

    DWORD now = GetTickCount();
    DeflectSlot* slot = nullptr;
    for (int i = 0; i < kMaxDeflectSlots; ++i) {
        if (g_deflect[i].used && strcmp(g_deflect[i].actor, actor) == 0) {
            slot = &g_deflect[i];
            break;
        }
    }
    if (!slot) {
        for (int i = 0; i < kMaxDeflectSlots; ++i) {
            if (!g_deflect[i].used) { slot = &g_deflect[i]; break; }
        }
        // Table full: flush the oldest to make room rather than dropping.
        if (!slot) {
            slot = &g_deflect[0];
            for (int i = 1; i < kMaxDeflectSlots; ++i) {
                if (g_deflect[i].first_tick < slot->first_tick) slot = &g_deflect[i];
            }
            FlushDeflect(*slot);
        }
        *slot = {};
        slot->used = true;
        memcpy(slot->actor, actor, strnlen(actor, sizeof(slot->actor) - 1) + 1);
        slot->first_tick = now;
    }
    ++slot->count;
    slot->last_tick = now;
    return true;  // claimed
}

void TickCombatAbsorb() {
    if (g_absorb_count == 0) return;
    DWORD now = GetTickCount();
    bool quiet  = (now - g_absorb_last_tick)  >= kAbsorbQuietMs;
    bool maxAge = (now - g_absorb_first_tick) >= kAbsorbMaxHoldMs;
    if (quiet || maxAge) FlushAbsorb();
}

void TickCombatDeflect() {
    DWORD now = GetTickCount();
    for (int i = 0; i < kMaxDeflectSlots; ++i) {
        if (!g_deflect[i].used) continue;
        bool quiet  = (now - g_deflect[i].last_tick)  >= kDeflectQuietMs;
        bool maxAge = (now - g_deflect[i].first_tick) >= kDeflectMaxHoldMs;
        if (quiet || maxAge) FlushDeflect(g_deflect[i]);
    }
}

}  // namespace acc::combat
