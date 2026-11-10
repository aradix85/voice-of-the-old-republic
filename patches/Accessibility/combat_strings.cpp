// The combat-log text of the English game: the engine phrases the combat
// rules parse (from dialog.tlk, each with its strref) and the shorter labels
// they speak. KOTOR 2's combat block is byte-identical to KOTOR 1's, so one
// table serves both games.
//
// BuildCompact speaks "<actor> hits critical <target>"; "critically hits"
// would need a BuildCompact change.
//
// Field order MUST match the declaration order in combat_strings.h
// (positional init).

#include "combat_strings.h"

namespace acc::combat::loc {

namespace {

const MsgStrings kEn = {
    // ---- Engine-side parse anchors (extracted from EN dialog.tlk)
    " succeeds with attack on ",   // phrase_hit       (template 42042 + verb 42043)
    " fails with attack on ",      // phrase_miss      (template 42042 + verb 42044)
    " with ",                      // phrase_mit       (42119; the hit/miss tag in
                                   //   <CUSTOM0>, strref 42133 "Hit" / 42134 "Miss",
                                   //   renders "...on <target>. Hit with N vs...")
    "Defense ",                    // word_verteidigung (42119 gap CUSTOM1..CUSTOM2)
    "damage ",                     // word_schaden_colon (42119 gap CUSTOM2..CUSTOM3)
    " used.",                      // feat_marker      (42046 + engine-appended ".")
    "Attack Breakdown: ",          // prefix_angriff   (42146)
    "Defense Breakdown: ",         // prefix_abwehr    (42149)
    "Damage Breakdown: ",          // prefix_schaden   (42150)
    "Threat Breakdown:",           // prefix_bedrohung (42148)
    "Critical Hit!",               // tag_krit_summary (1511)
    "Automatic Hit!",              // tag_auto_hit     (42390)
    "Automatic Miss!",             // tag_auto_fail    (42391)
    "roll ",                       // token_wuerfel    (42316, parser pre-strips leading space)
    "dex mod ",                    // token_gesch_mod  (42339, attack-context label)
    "Close Proximity Ranged Bonus ", // token_entfernung (42330)
    "Effect Bonus ",               // token_effekt     (42332)
    "Critical x",                  // krit_x_prefix    (42386)
    " for ",                       // phrase_fuer      (42386 tail after CUSTOM0)
    "bonus damage",                // token_bonusschaden (42155)
    "Mainhand",                    // hand_main        (42314)

    // ---- Output-side labels (hand-translated)
    "hits",                        // verb_hit
    "misses",                      // verb_miss
    "critical",                    // word_critical (awkward EN word order — see header)
    "Attack",                      // word_angriff
    "Def.",                        // word_vert
    "vs.",                         // word_gg
    "Damage",                      // word_schaden
    "from",                        // word_von
    "Auto-hit.",                   // word_auto_hit
    "Auto-miss.",                  // word_auto_fail

    // ---- Short replacements (compaction of engine breakdown tokens)
    "R",                           // short_wuerfel    (roll → R)
    "Dex ",                        // short_gesch
    "Close-range ",                // short_reichweite (engine label is verbose "Close Proximity Ranged Bonus")
    "Effect ",                     // short_effekt
    "Bonus",                       // short_bonus

    // ---- Short results-only line labels
    "failed",                            // word_failed
    "Effect Application Breakdown:",     // prefix_auswirkung (TLK 42157)
    "absorbs",                           // absorb_anchor (1455 verb; misses 1454 "resists" energy-shield variant)

    // ---- Ability / grenade / force-power effect sequence
    " uses ",                            // ability_use_marker (32292)
    " Save. ",                           // save_marker (1374-1376 common suffix + 1406 separator)
    "success!",                          // save_success (1392 + 1406 punct)
    "failure!",                          // save_fail (1393 + 1406 punct)
    " damages ",                         // damage_marker (1403)
    "resists",                           // word_resists
    "failed",                            // word_save_failed
    "killed",                            // kill_marker (1407)

    // ---- Status-echo copula
    " is ",                              // status_ist_marker

    // ---- Blaster-deflection breakdown (42417)
    "Deflection Breakdown: ",            // prefix_reflexion
    " deflects projectile with ",        // reflect_mid_marker
    "%s deflects 1 shot",                // fmt_deflect_one
    "%s deflects %d shots",              // fmt_deflect_many
};

}  // namespace

const MsgStrings& Get() { return kEn; }

}  // namespace acc::combat::loc
