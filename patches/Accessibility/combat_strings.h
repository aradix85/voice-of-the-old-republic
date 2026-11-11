// Combat-log text: the engine phrases the combat rules parse (anchors) and
// the shorter labels they speak. The values, with their dialog.tlk strrefs,
// are in combat_strings.cpp; an anchor that does not match leaves the line on
// the raw-speech fallback. The field names are the German words the table
// was first written for.

#pragma once

namespace acc::combat::loc {

struct MsgStrings {
    // ---- Engine-side parse anchors. Each must match the engine's text
    //      exactly (case, spaces, trailing punctuation).
    const char* phrase_hit;
    const char* phrase_miss;
    const char* phrase_with;
    const char* word_defense;
    const char* word_damage_colon;
    const char* feat_marker;
    const char* prefix_attack;
    const char* prefix_defense;
    const char* prefix_damage;
    const char* prefix_threat;
    const char* tag_crit_summary;
    const char* tag_auto_hit;
    const char* tag_auto_fail;
    const char* token_roll;
    const char* token_dex_mod;
    const char* token_range;
    const char* token_effect;
    const char* crit_x_prefix;
    const char* phrase_for;
    const char* token_bonus_damage;
    const char* hand_main;           // default hand label, left out of the output

    // ---- Output-side labels (what we speak).
    const char* verb_hit;
    const char* verb_miss;
    const char* word_critical;
    const char* word_attack;
    const char* word_def;
    const char* word_vs;             // separator "vs."
    const char* word_damage;
    const char* word_from;
    const char* word_auto_hit;
    const char* word_auto_fail;

    // Shorter replacements for the verbose breakdown labels above, so
    // "roll 14+ ..." is spoken "R14+ ...".
    const char* short_roll;       // no trailing space: the value follows
    const char* short_dex;
    const char* short_range;
    const char* short_effect;
    const char* short_bonus;         // no trailing space: " 5" follows

    // ---- Results-only line labels.
    const char* word_failed;         // miss line: "<actor>, failed, <feat>"

    // The engine repeats an applied status ("Effect Application Breakdown:
    // <target> is <status>"). The status is already on the attack line, so
    // this line is claimed and suppressed.
    const char* prefix_effect_application;

    // Damage absorption ("<name> absorbs N damage"): one line per blocked
    // hit, so a burst is coalesced into one debounced total.
    const char* absorb_anchor;

    // ---- Ability, grenade and Force power events, merged per target:
    //   use:    "<actor> uses <action>."
    //   save:   "<saver> <SaveType> Save. success!|failure! ..."
    //   damage: "<actor> damages <target>: N damage (Type: N)"
    //   status: "Effect Application Breakdown:<target> is <status>"
    const char* ability_use_marker;
    const char* save_marker;
    const char* save_success;
    const char* save_fail;
    const char* damage_marker;
    const char* word_resists;        // save-success verb
    const char* word_save_failed;    // save-failure tag

    // Kill line ("<actor> killed <target>").
    const char* kill_marker;

    // The copula in "<target> is <status>": lifts the status word onto the
    // attack or effect line and splits the status echo.
    const char* status_is_marker;

    // ---- Blaster deflection (strref 42417): one breakdown per deflected
    // shot, coalesced into "<actor> deflects N shots" (party members only).
    const char* prefix_deflection;
    const char* reflect_mid_marker;
    const char* fmt_deflect_one;
    const char* fmt_deflect_many;
};

const MsgStrings& Get();

}  // namespace acc::combat::loc
