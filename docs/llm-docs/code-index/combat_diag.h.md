# combat_diag.h (49 lines)

Source: `patches/Accessibility/combat_diag.h`

Combat-queue diagnostics: log lines only, under "Combat.Diag". No engine state is changed.

The state they follow decides chain versus overwrite in DoPersonalAction / DoTargetAction:

cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit) qs  = CSWSCombatRound.actions count        (the round's queue) oq  = CSWSObject.action_nodes count        (the outer action queue) ap  = CClientExoApp::GetAutoPaused() ps  = CClientExoApp::GetPauseState(0) tgt = main_interface.field1_0x64           (current target handle)

## Declarations (in source order)

- L20 `void Tick()`: Per frame: a DELTA line for every tracked value that changed.
- L25 `void LogPreFire(const char* label)`: The state right before and right after a combat key is dispatched.
- L26 `void LogPostFire(const char* label)`

Also 1 constants or variables at namespace scope.
