# combat_diag.h (81 lines)

Source: `patches/Accessibility/combat_diag.h`

Combat-queue diagnostic probe.

Per-press snapshot + per-tick delta of the four state bits that decide chain vs overwrite in DoPersonalAction / DoTargetAction:

cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit) qs  = CSWSCombatRound.actions list size   (queue depth, ignoring 0xFF placeholder) ap  = CClientExoApp::GetAutoPaused() ps  = CClientExoApp::GetPauseState(0) tgt = main_interface.field1_0x64           (current target handle)

Read-only — no engine state is mutated. Logs to acclog under "Combat.Diag". Off when not in a world (no player position).

Wiring points: * Tick()              — per-frame from core_tick. Emits a DELTA line whenever any tracked bit changes. * LogPreFire(label)   — called from each combat-key press path BEFORE the engine dispatches. * LogPostFire(label)  — called AFTER the engine returns (Shift+1..3 and Shift+4..7 only; bare 1..7 fires after our prologue hook returns, so we get the "post" state from the next Tick's DELTA line).

## Declarations (in source order)

- L34 `void Tick()`: Also speaks the idle notice (KOTOR 1): in combat, nothing queued for the controlled character and an enemy near for 2.5 s -> "You are not attacking..." once, until it attacks again or combat ends.
- L39 `bool LeaderIsAttacking(uint32_t targetId)`: True when the controlled character's first queued combat action targets `targetId` (client or server id; the high bit is ignored).
- L43 `void LogPreFire(const char* label)`: `label` is a short tag: "bare-1", "bare-5", "shift+1-enter", "shift+5-enter".
- L44 `void LogPostFire(const char* label)`

Also 1 constants or variables at namespace scope.
