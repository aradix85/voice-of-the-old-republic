// Combat narration: the combat-mode cues (combat.cpp) and the flushes of the
// combat log's merged lines (combat_log_shields.cpp, combat_log_effects.cpp).
// The log itself is narrated from the OnAppendToMsgBuffer hook.

#pragma once

namespace acc::combat {

// Cross-feature gating read — false on chain fault (treat as not-in-combat).
// IsCombatActive mirrors only the *controlled leader's* combat bit, so a Tab to
// a not-yet-engaged member reads peace mid-encounter. IsPartyInCombat is the
// encounter-level truth (OR of every party member's per-creature combat bit),
// immune to which member is controlled — the same signal TickCombatMode's
// begin/end cue and menu auto-close already trust. Prefer it for anything that
// must not flip just because the player switched leaders during a fight.
bool IsCombatActive();
bool IsPartyInCombat();

void TickCombatMode();

// A kill line of the combat log ("<actor> killed <target>: N XP"), kept for
// the summary that "Combat ends" speaks.
void NoteKillLine(const char* line);

// Flushes a debounced burst of damage-absorption lines into one spoken
// total. Cheap and idle when no absorb burst is pending.
void TickCombatAbsorb();

// Flushes debounced blaster-deflection bursts into one spoken line per
// deflector ("<actor> deflects N shots"). Cheap and idle when none
// is pending.
void TickCombatDeflect();

// Flushes debounced per-target ability/grenade/force-power effects into one
// merged spoken line each. Cheap and idle when no effect is pending.
void TickCombatEffects();

}  // namespace acc::combat
