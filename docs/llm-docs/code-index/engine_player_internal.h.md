# engine_player_internal.h (35 lines)

Source: `patches/Accessibility/engine_player_internal.h`

internal seam between the three TUs the player module is split across.

This is NOT public API. engine_player.h stays the public surface; this header exists only because the Phase-1 structure pass (refactoring candidate 5) split engine_player.cpp into:

engine_player.cpp            core position / facing / camera reads engine_player_party.cpp      party + leader resolution engine_player_inputlock.cpp  input-disable + auto-restore state machine

GetPlayerServerObject was file-static while all three lived in one TU and is used by all three, so it is the one thing that had to be published. The typedefs travel with it because they describe the same chain walk.

## Declarations (in source order)

- L19 `typedef void* (__thiscall* PFN_GetPlayerCreature)(void* this_)`
- L20 `typedef void* (__thiscall* PFN_CSWSObjectGetArea)(void* this_)`
- L21 `typedef void* (__thiscall* PFN_GetPlayerCharacterName)(void* this_)`
- L32 `void* GetPlayerServerObject()`: Walk *kAddrAppManagerPtr -> CClientExoApp -> GetPlayerCreature() -> server_object (+0xf8) and return the CSWSObject*.
