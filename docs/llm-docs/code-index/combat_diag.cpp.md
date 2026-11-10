# combat_diag.cpp (342 lines)

Source: `patches/Accessibility/combat_diag.cpp`

(From combat_diag.h.) Combat-queue diagnostic probe.

Per-press snapshot + per-tick delta of the four state bits that decide chain vs overwrite in DoPersonalAction / DoTargetAction:

cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit) qs  = CSWSCombatRound.actions list size   (queue depth, ignoring 0xFF placeholder) ap  = CClientExoApp::GetAutoPaused() ps  = CClientExoApp::GetPauseState(0) tgt = main_interface.field1_0x64           (current target handle)

Read-only — no engine state is mutated. Logs to acclog under "Combat.Diag". Off when not in a world (no player position).

Wiring points: * Tick()              — per-frame from core_tick. Emits a DELTA line whenever any tracked bit changes. * LogPreFire(label)   — called from each combat-key press path BEFORE the engine dispatches. * LogPostFire(label)  — called AFTER the engine returns (Shift+1..3 and Shift+4..7 only; bare 1..7 fires after our prologue hook returns, so we get the "post" state from the next Tick's DELTA line).

## Declarations (in source order)

- L56 `uint8_t ReadCombatModeBit(void* clientLeader)`
- L67 `int CallGetAutoPaused(void* exoApp)`
- L78 `int CallGetPauseState(void* exoApp)`
- L89 `uint32_t ReadMainInterfaceTarget(void* exoApp)`
- L113 `void ReadState(State& s)`
- L139 `void Tick()`
- L185 `void LogPreFire(const char* label)`
- L193 `void LogPostFire(const char* label)`
- L204 `extern "C" void __cdecl OnCombatRoundRemoveAllActions(void* this_combatRound)`
- L239 `extern "C" void __cdecl OnCombatRoundSetCurrentAction(void* this_combatRound, void* esp_byte_addr)`
- L254 `extern "C" void __cdecl OnCombatRoundRemoveLastAction(void* this_combatRound)`
- L273 `void __cdecl OnCombatRoundRemoveAllActionsK2(void* thisRound)`
- L282 `void __cdecl OnCombatRoundSetCurrentActionK2(void* thisRound, void* ebp)`
- L291 `void __cdecl OnCombatRoundRemoveLastActionK2(void* thisRound)`
- L310 `void __cdecl OnUseItemRequestK2(void* thisPtr, void* ebp)`
