# engine_player_party.cpp (552 lines)

Source: `patches/Accessibility/engine_player_party.cpp`

party and leader resolution.

Split out of engine_player.cpp by the Phase-1 structure pass (refactoring candidate 5). Everything here answers "who is in the party, who is leading, what are they called": leader/creature resolution, the party table accessors, solo mode, roster availability/selectability and the companion name table.

The functions were interleaved with the input-lock state machine in the original file, so this is a two-piece cut rather than one slice - the definitions themselves are verbatim.

Declarations stay in engine_player.h; GetPlayerServerObject comes from engine_player_internal.h.

## Declarations (in source order)

- L32 `void* GetPlayerServerCreature()`
- L36 `void* GetClientLeader()`
- L48 `void* GetPartyCreatureBySlotK2(int slot)`
- L70 `int SetLeaderQueueModeBit(int on)`
- L90 `bool IsAnyPartyMemberInCombat()`
- L130 `bool GetActiveLeaderName(char* outBuf, size_t bufSize)`
- L275 `bool GetPlayerCharacterName(char* outBuf, size_t bufSize)`
- L294 `int GetPartyMembers(uint32_t* outHandles, int maxCount)`
- L337 `void* GetServerPartyTable()`
- L352 `bool GetSoloMode()`
- L372 `int PartyRosterSlotCount()`
- L376 `bool IsPartyPortrait(void* control)`
- L387 `bool ReadPartyPortrait(void* control, PartyPortrait* out)`
- L409 `int PartySelectionChosenCount(void* panel)`
- L420 `bool PartyTableIsNPCAvailable(int npcSlot)`
- L433 `bool PartyTableIsNPCSelectable(int npcSlot)`
- L467 `bool GetPartyNpcNameForSlot(int npcSlot, char* outBuf, size_t bufSize)`
