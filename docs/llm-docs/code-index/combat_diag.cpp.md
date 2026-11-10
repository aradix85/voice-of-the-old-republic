# combat_diag.cpp (263 lines)

Source: `patches/Accessibility/combat_diag.cpp`

(From combat_diag.h.) Combat-queue diagnostics: log lines only, under "Combat.Diag". No engine state is changed.

The state they follow decides chain versus overwrite in DoPersonalAction / DoTargetAction:

cm  = CSWCCreature.field200_0x440 & 1     (combat-mode bit) qs  = CSWSCombatRound.actions count        (the round's queue) oq  = CSWSObject.action_nodes count        (the outer action queue) ap  = CClientExoApp::GetAutoPaused() ps  = CClientExoApp::GetPauseState(0) tgt = main_interface.field1_0x64           (current target handle)

## Declarations (in source order)

- L44 `uint8_t ReadCombatModeBit(void* clientLeader)`
- L50 `int CallGetAutoPaused(void* exoApp)`
- L61 `int CallGetPauseState(void* exoApp)`
- L82 `State ReadState()`
- L98 `void LogState(const char* phase, const char* label, const State& s)`
- L103 `void LogDelta(const char* what, int before, int after)`
- L114 `void Tick()`
- L139 `void LogPreFire(const char* label)`
- L140 `void LogPostFire(const char* label)`
- L144 `extern "C" void __cdecl OnCombatRoundRemoveAllActions(void* this_combatRound)`
- L165 `extern "C" void __cdecl OnCombatRoundSetCurrentAction(void* this_combatRound, void* esp_byte_addr)`
- L176 `extern "C" void __cdecl OnCombatRoundRemoveLastAction(void* this_combatRound)`
- L194 `void __cdecl OnCombatRoundRemoveAllActionsK2(void* thisRound)`
- L203 `void __cdecl OnCombatRoundSetCurrentActionK2(void* thisRound, void* ebp)`
- L212 `void __cdecl OnCombatRoundRemoveLastActionK2(void* thisRound)`
- L231 `void __cdecl OnUseItemRequestK2(void* thisPtr, void* ebp)`
