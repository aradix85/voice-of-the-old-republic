// The combat log as speech: the rules, registered with acc::msg::Router, that
// turn the engine's combat-log lines into the mod's short spoken lines. The
// 3-or-4-line attack sequence (summary, Attack Breakdown, [Threat Breakdown],
// Defense Breakdown, [Damage Breakdown]) becomes one compact line
// (combat_log_attack.cpp); abilities, shields and deflection have their own
// files. Everything else (heals, item use, loot, world feedback) falls
// through to the router's raw-speech fallback. Here: the registry, the shared
// text helpers, and the one-line rules (a kill, a detected mine).
//
// Parsing is anchor-based against the engine strings in combat_strings.*;
// the anchors are exact (spacing and punctuation included) and must not be
// "tidied".

#include "combat_log_internal.h"

#include "combat_strings.h"
#include "engine_compass.h"   // ClockPosition
#include "msg_router.h"
#include "speech.h"
#include "strfmt.h"           // Format
#include "strings.h"
#include "trap_watch.h"       // the fresh mine detection
#include "view_mode.h"        // GetEffectiveViewpoint: the camera frame

namespace acc::combat {

namespace {

// "<actor> killed <target>: N XP" — a defeat. Spoken verbatim and
// interrupting, so a kill cuts through queued combat chatter instead of
// waiting behind it.
bool RuleKill(const char* text) {
    if (!strstr(text, acc::combat::loc::Get().kill_marker)) return false;
    acc::speech::Speak(text, /*interrupt=*/true);
    return true;  // claimed: speech handled here
}

// Mine-detection feedback enrichment. The engine line (TLK 42132,
// "<detector> detects <mine>: Awareness <roll> vs DC <dc>")
// tells the player a mine exists but not WHERE — sighted players see the
// red ground overlay appear at the same moment. trap_watch records every
// fresh party mine detection (name + position); when a message-buffer
// line arrives that contains that mine's name, it is the detect line, so
// no anchor phrase is needed. Rewrite:
// keep the text up to the roll-math colon (the math is review-log
// detail, same reasoning as RuleThreat) and append clock direction +
// distance via the shared announce format:
//   "<detect line>, 2 o'clock, 6 metres"
bool RuleMineDetect(const char* text) {
    char   mineName[96];
    Vector minePos;
    if (!acc::trap_watch::PeekFreshMine(mineName, sizeof(mineName),
                                        minePos)) {
        // The engine can append the feedback line before our throttled
        // scan noticed the detected-by transition — force one rescan
        // (rate-limited inside) before concluding this isn't a detect line.
        acc::trap_watch::ScanNow();
        if (!acc::trap_watch::PeekFreshMine(mineName, sizeof(mineName),
                                            minePos)) {
            return false;
        }
    }
    if (!mineName[0] || !strstr(text, mineName)) return false;
    acc::trap_watch::ConsumeFreshMine();

    // Prefix = everything before the stat separator (the last colon —
    // the roll math after it carries no colon of its own). Fall back to
    // the whole line if the shape is unexpected.
    char prefix[256];
    const char* colon = strrchr(text, ':');
    size_t plen = colon ? (size_t)(colon - text) : strlen(text);
    if (plen >= sizeof(prefix)) plen = sizeof(prefix) - 1;
    memcpy(prefix, text, plen);
    prefix[plen] = '\0';

    std::string line(prefix);
    // Camera-anchored viewpoint — see trap_watch::WithClock for why the
    // creature's own facing is the wrong frame for a spoken clock.
    Vector p{};
    float  yaw = 0.0f;
    if (acc::view_mode::GetEffectiveViewpoint(p, yaw)) {
        int clock = acc::engine::ClockPosition(yaw, minePos.x - p.x,
                                               minePos.y - p.y);
        int m = static_cast<int>(DistXY(p, minePos) + 0.5f);
        if (m < 1) m = 1;
        const char* fmt =
            acc::strings::Get(acc::strings::Id::FmtAnnounceWithClock);
        if (fmt && fmt[0]) line = acc::strfmt::Format(fmt, prefix, clock, m);
    }

    auto& r = acc::msg::Router::Instance();
    r.Speak(line.c_str());
    r.LogEmit("emit-mine-detect", line.c_str());
    return true;  // claimed
}

}  // namespace

bool MsgStartsWith(const char* s, const char* p) {
    while (*p) { if (*s++ != *p++) return false; }
    return true;
}

void CopyRange(char* dst, size_t cap, const char* start, const char* end) {
    while (start < end && *start == ' ') ++start;
    while (end > start && (end[-1] == ' ' || end[-1] == '.')) --end;
    size_t n = (size_t)(end - start);
    if (n >= cap) n = cap - 1;
    memcpy(dst, start, n);
    dst[n] = '\0';
}

void RegisterCombatMsgRules() {
    auto& r = acc::msg::Router::Instance();
    // Mine-detect first: it only activates while a fresh detection is
    // queued (cheap peek), and its line shape ("<actor> detects <mine>:
    // ...") must not fall through to the generic rules.
    r.AddRule("CombatMineDetect", RuleMineDetect);
    r.AddRule("CombatSummary",   RuleSummary);
    r.AddRule("CombatAttack",    RuleAttack);
    r.AddRule("CombatDefense",   RuleDefense);
    r.AddRule("CombatDamage",    RuleDamage);
    r.AddRule("CombatEffectApplication", RuleEffectApplication);
    r.AddRule("CombatAbsorb",    RuleAbsorb);
    r.AddRule("CombatDeflect",   RuleDeflect);
    r.AddRule("CombatAbilityUse", RuleAbilityUse);
    r.AddRule("CombatSaveThrow", RuleSaveThrow);
    r.AddRule("CombatDirectDamage", RuleDirectDamage);
    r.AddRule("CombatKill",      RuleKill);
    r.AddRule("CombatThreat",    RuleThreat);
    r.AddOnUnmatched(OnUnmatched);
}

}  // namespace acc::combat
