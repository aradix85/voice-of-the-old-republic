# combat_queue_hooks.cpp (57 lines)

Source: `patches/Accessibility/combat_queue_hooks.cpp`

CSWSCombatRound::AddAction detour: the trigger of the combat-queue announce ("X, position N" / "X, queue full"). Every genuine engine action-add is forwarded to acc::combat::queue::OnEngineActionAdded. The log-only CSWSCombatRound hooks are in combat_diag.cpp.

## Declarations (in source order)

- L16 `extern "C" void __cdecl OnCombatRoundAddAction(void* this_combatRound, void* esp_action_addr, void* esp_param2_addr)`
- L50 `void __cdecl OnCombatRoundAddActionK2(void* thisRound, void* ebp)`
