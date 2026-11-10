// Internal to the combat_log module (combat_log*.cpp); no other file includes
// it. The public interface is combat.h. Here: what the files share.

#pragma once

#include "combat.h"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace acc::combat {

// One attack sequence as it is collected: it opens on a summary line and
// closes on the Damage Breakdown (hit) or on any unrecognised line (miss).
struct AttackBlock {
    bool has_summary;
    bool has_attack;
    bool has_defense;
    bool has_damage;
    bool hit;
    bool crit_tag;       // "Critical Hit!" in summary
    bool auto_hit;       // "Automatic Hit!"
    bool auto_fail;      // "Automatic Miss!"
    char actor[96];
    char target[96];
    char feat[96];       // optional, e.g. "Power Attack"
    int  sum_attack;
    int  sum_defense;
    int  sum_damage;
    char attack_hand[32];     // "Mainhand" / off hand
    char attack_comps[256];   // pre-formatted: "R14+Dex 2+Effect 1+..."
    char defense_nums[64];      // pre-formatted: "10+3+2" (labels dropped)
    char damage_text[128];    // full-breakdown form: "Energy 6+Bonus 5 x2"
    char damage_short[128];   // short-form: "6 Energy + 5 Bonus"  (value first, " + " separator)
    char dmg_type[32];         // primary damage type label, e.g. "Energy" / "Physical"
    char status[96];           // applied status from summary tail, e.g. "stunned" ("" if none)
};

// Helpers shared across the files, by the file that defines them.

// combat_log.cpp
bool MsgStartsWith(const char* s, const char* p);
void CopyRange(char* dst, size_t cap, const char* start, const char* end);

// combat_log_parse.cpp
bool ParseSummary(const char* text, AttackBlock& b);
bool ParseAttack(const char* text, AttackBlock& b);
bool ParseDefense(const char* text, AttackBlock& b);
bool ParseDamage(const char* text, AttackBlock& b);

// combat_log_attack.cpp
bool MaybeSpeakStatusUrgent(const char* victim, const char* status);
bool RuleSummary(const char* text);
bool RuleAttack(const char* text);
bool RuleDefense(const char* text);
bool RuleDamage(const char* text);
bool RuleThreat(const char* text);
void OnUnmatched(const char* );

// combat_log_effects.cpp
bool RuleEffectApplication(const char* text);
bool RuleAbilityUse(const char* text);
bool RuleSaveThrow(const char* text);
bool RuleDirectDamage(const char* text);

// combat_log_shields.cpp
bool RuleAbsorb(const char* text);
bool RuleDeflect(const char* text);

}  // namespace acc::combat
