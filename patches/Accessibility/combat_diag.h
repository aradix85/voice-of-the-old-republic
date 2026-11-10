// Combat-queue diagnostics: log lines only, under "Combat.Diag". No engine
// state is changed.
//
// The state they follow decides chain versus overwrite in DoPersonalAction /
// DoTargetAction:
//
//   cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit)
//   qs  = CSWSCombatRound.actions count        (the round's queue)
//   oq  = CSWSObject.action_nodes count        (the outer action queue)
//   ap  = CClientExoApp::GetAutoPaused()
//   ps  = CClientExoApp::GetPauseState(0)
//   tgt = main_interface.field1_0x64           (current target handle)

#pragma once

namespace acc::combat_diag {

// Per frame: a DELTA line for every tracked value that changed. Silent
// outside a world.
void Tick();

// The state right before and right after a combat key is dispatched. `label`
// names the press ("bare-1", "menu-enter"). A bare 1..7 fires after our
// prologue hook returns, so its "after" is the next Tick's DELTA line.
void LogPreFire(const char* label);
void LogPostFire(const char* label);

}  // namespace acc::combat_diag

// Detours into the combat-round mutators, for deterministic ADD / CLEAR
// events rather than a list walk that races the engine. Each line carries the
// round and whether it is the player's.
extern "C" {
    // AddAction @0x4d3660: the generic adder behind every Add*Action. Also
    // the trigger of the spoken queue announce (combat_queue_hooks.cpp).
    void __cdecl OnCombatRoundAddAction(void* this_combatRound,
                                        void* esp_action_addr,
                                        void* esp_param2_addr);
    // RemoveAllActions @0x4d3770: the engine's "overwrite" primitive.
    void __cdecl OnCombatRoundRemoveAllActions(void* this_combatRound);
    // SetCurrentAction(byte): an action moves into the "currently
    // dispatching" slot at +0x9d0; once per dispatched action.
    void __cdecl OnCombatRoundSetCurrentAction(void* this_combatRound,
                                               void* esp_byte_addr);
    // RemoveLastAction(): pops the tail of the queue. Seen after a press, it
    // is the engine culling queued entries.
    void __cdecl OnCombatRoundRemoveLastAction(void* this_combatRound);
}
