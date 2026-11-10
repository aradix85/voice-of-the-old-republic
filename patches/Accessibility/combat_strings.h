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
    const char* phrase_mit;
    const char* word_verteidigung;
    const char* word_schaden_colon;
    const char* feat_marker;
    const char* prefix_angriff;
    const char* prefix_abwehr;
    const char* prefix_schaden;
    const char* prefix_bedrohung;
    const char* tag_krit_summary;
    const char* tag_auto_hit;
    const char* tag_auto_fail;
    const char* token_wuerfel;
    const char* token_gesch_mod;
    const char* token_entfernung;
    const char* token_effekt;
    const char* krit_x_prefix;
    const char* phrase_fuer;
    const char* token_bonusschaden;
    const char* hand_main;           // default hand label, left out of the output

    // ---- Output-side labels (what we speak).
    const char* verb_hit;
    const char* verb_miss;
    const char* word_critical;
    const char* word_angriff;
    const char* word_vert;
    const char* word_gg;             // separator "vs."
    const char* word_schaden;
    const char* word_von;
    const char* word_auto_hit;
    const char* word_auto_fail;

    // Shorter replacements for the verbose breakdown labels above, so
    // "roll 14+ ..." is spoken "R14+ ...".
    const char* short_wuerfel;       // no trailing space: the value follows
    const char* short_gesch;
    const char* short_reichweite;
    const char* short_effekt;
    const char* short_bonus;         // no trailing space: " 5" follows

    // ---- Results-only line labels.
    const char* word_failed;         // miss line: "<actor>, failed, <feat>"

    // The engine repeats an applied status ("Effect Application Breakdown:
    // <target> is <status>"). The status is already on the attack line, so
    // this line is claimed and suppressed.
    const char* prefix_auswirkung;

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

    // Kill line ("<actor> killed <target>"), spoken interrupting.
    const char* kill_marker;

    // The copula in "<target> is <status>": lifts the status word onto the
    // attack or effect line and splits the status echo.
    const char* status_ist_marker;

    // ---- Blaster deflection (strref 42417): one breakdown per deflected
    // shot, coalesced into "<actor> deflects N shots" (party members only).
    const char* prefix_reflexion;
    const char* reflect_mid_marker;
    const char* fmt_deflect_one;
    const char* fmt_deflect_many;
};

const MsgStrings& Get();

}  // namespace acc::combat::loc
