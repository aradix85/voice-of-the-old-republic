# combat_diag.cpp (544 lines)

Source: `patches/Accessibility/combat_diag.cpp`

(From combat_diag.h.) Combat-queue diagnostic probe.

Per-press snapshot + per-tick delta of the four state bits that decide chain vs overwrite in DoPersonalAction / DoTargetAction:

cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit) qs  = CSWSCombatRound.actions list size   (queue depth, ignoring 0xFF placeholder) ap  = CClientExoApp::GetAutoPaused() ps  = CClientExoApp::GetPauseState(0) tgt = main_interface.field1_0x64           (current target handle)

Read-only — no engine state is mutated. Logs to acclog under "Combat.Diag". Off when not in a world (no player position).

Wiring points: * Tick()              — per-frame from core_tick. Emits a DELTA line whenever any tracked bit changes. * LogPreFire(label)   — called from each combat-key press path BEFORE the engine dispatches. * LogPostFire(label)  — called AFTER the engine returns (Shift+1..3 and Shift+4..7 only; bare 1..7 fires after our prologue hook returns, so we get the "post" state from the next Tick's DELTA line).

## Declarations (in source order)

- L57 `uint8_t ReadCombatModeBit(void* clientLeader)`
- L76 `int ReadRoundActionCount(void* combatRound)`: Queued-action count on a CSWSCombatRound directly.
- L100 `int ReadQueueSize(void* serverCreature)`
- L121 `int ReadOuterQueueSize(void* serverCreature)`: Read the OUTER game-action queue size at CSWSObject.action_nodes @+0xfc.
- L144 `uint32_t ReadFirstQueuedTarget(void* serverCreature)`: Target (server id) of the first real action queued on the creature's combat round; 0 when nothing is queued or the read faults.
- L182 `bool HostileNearby()`
- L208 `int CallGetAutoPaused(void* exoApp)`
- L219 `int CallGetPauseState(void* exoApp)`
- L230 `uint32_t ReadMainInterfaceTarget(void* exoApp)`
- L254 `void ReadState(State& s)`
- L280 `void Tick()`
- L354 `bool LeaderIsAttacking(uint32_t targetId)`
- L361 `void LogPreFire(const char* label)`
- L369 `void LogPostFire(const char* label)`
- L383 `void* GetPlayerCombatRound()`: Resolve the player's CSWSCombatRound at hook-fire time so we can tag each ADD/CLEAR event with whether it belongs to the user's creature (vs companion / enemy combat rounds, which also use the same engine surfaces).
- L399 `const char* RoleTag(void* combatRound)`: Declared in combat_diag_internal.h - combat_queue_hooks.cpp tags its log lines with the same role, so this one helper is not file-static.
- L406 `extern "C" void __cdecl OnCombatRoundRemoveAllActions(void* this_combatRound)`
- L441 `extern "C" void __cdecl OnCombatRoundSetCurrentAction(void* this_combatRound, void* esp_byte_addr)`
- L456 `extern "C" void __cdecl OnCombatRoundRemoveLastAction(void* this_combatRound)`
- L475 `void __cdecl OnCombatRoundRemoveAllActionsK2(void* thisRound)`
- L484 `void __cdecl OnCombatRoundSetCurrentActionK2(void* thisRound, void* ebp)`
- L493 `void __cdecl OnCombatRoundRemoveLastActionK2(void* thisRound)`
- L512 `void __cdecl OnUseItemRequestK2(void* thisPtr, void* ebp)`
