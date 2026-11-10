# combat_diag_internal.h (23 lines)

Source: `patches/Accessibility/combat_diag_internal.h`

internal seam between combat_diag.cpp and combat_queue_hooks.cpp.

This is NOT public API. The Phase-1 structure pass (refactoring candidate 14) moved OnCombatRoundAddAction out of combat_diag.cpp because it is not a diagnostic: it drives the shipped combat-queue announce ("X, position N" / "X, queue full") by forwarding to acc::combat::queue::OnEngineActionAdded. The three genuinely diagnostic CSWSCombatRound hooks stayed behind.

Both files tag their log lines with the same PLAYER/other role, so that one helper is published here.

## Declarations (in source order)

- L20 `const char* RoleTag(void* combatRound)`: "PLAYER" when `combatRound` belongs to the user's creature, "other" for companion / enemy rounds sharing the same engine surfaces.
