# combat_queue_hooks.cpp (93 lines)

Source: `patches/Accessibility/combat_queue_hooks.cpp`

CSWSCombatRound::AddAction detour - the combat-queue announce trigger.

Split out of combat_diag.cpp by the Phase-1 structure pass (refactoring candidate 14). This hook is NOT a diagnostic despite having lived in a file named diag: it is the mechanism behind the shipped queue announce ("X, Platz N" / "Warteschlange voll"), forwarding every genuine engine action-add to acc::combat::queue::OnEngineActionAdded. It sits next to combat_queue.cpp so that relationship is visible from the filename.

The three CSWSCombatRound hooks that really are log-only (RemoveAllActions / SetCurrentAction / RemoveLastAction) stayed in combat_diag.cpp.

Hook wiring (hooks.toml + exports.def) is unchanged - only the definition's home file moved.

## Declarations (in source order)

- L35 `extern "C" void __cdecl OnCombatRoundAddAction(void* this_combatRound, void* esp_action_addr, void* esp_param2_addr)`
- L86 `void __cdecl OnCombatRoundAddActionK2(void* thisRound, void* ebp)`
