# engine_player_inputlock.cpp (221 lines)

Source: `patches/Accessibility/engine_player_inputlock.cpp`

player input-disable state machine and its auto-restore watchdog.

Split out of engine_player.cpp by the Phase-1 structure pass (refactoring candidate 5). SetPlayerInputEnabled hands control of the player to a scripted/queued action; the watchdog here restores it when the action queue drains, stalls, or becomes unreadable - so a failed dispatch can never leave the player permanently unable to move.

The functions were interleaved with the party readers in the original file, so this is a two-piece cut rather than one slice - the definitions themselves are verbatim, including the session statics and the four timing constants the watchdog is tuned around.

Declarations stay in engine_player.h; GetPlayerServerObject comes from engine_player_internal.h.

## Declarations (in source order)

- L66 `void* GetPlayerControl()`: CClientExoAppInternal → CSWPlayerControl.
- L85 `int GetPlayerActionQueueDepth()`: Player's pending AI-action count (CSWSObject.action_nodes @+0xfc → internal->count @+0x8).
- L100 `bool SetPlayerInputEnabled(bool enabled, bool armAutoRestore)`
- L146 `void TickPlayerInputRestore()`
- L204 `void TickActionQueueDiag()`
