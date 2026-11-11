# engine_player_party.cpp (378 lines)

Source: `patches/Accessibility/engine_player_party.cpp`

Who is in the party, who is leading, and what they are called: leader and creature resolution, the party table, solo mode, roster availability and the companion names. Declarations are in engine_player.h.

## Declarations (in source order)

- L21 `void* GetPlayerServerCreature()`
- L25 `void* GetClientLeader()`
- L37 `void* GetPartyCreatureBySlotK2(int slot)`
- L59 `int SetLeaderQueueModeBit(int on)`
- L74 `bool IsAnyPartyMemberInCombat()`
- L122 `uint32_t NpcObjectId(void* table, int slot)`: The object id of the creature in a roster slot (an index: 2 = Carth, 6 = Mission), 0 when none is live.
- L136 `bool StatsFirstName(void* serverCreature, char* outBuf, size_t bufSize)`: A server creature's stats.first_name, a pure memory read.
- L148 `bool GetActiveLeaderName(char* outBuf, size_t bufSize)`: The name of the CURRENTLY CONTROLLED leader: Tab re-wires CClientExoApp::GetPlayerCreature to the new one.
- L197 `bool GetPlayerCharacterName(char* outBuf, size_t bufSize)`
- L216 `int GetPartyMembers(uint32_t* outHandles, int maxCount)`
- L241 `void* GetServerPartyTable()`
- L249 `bool GetSoloMode()`
- L262 `int PartyRosterSlotCount()`
- L266 `bool IsPartyPortrait(void* control)`
- L270 `bool ReadPartyPortrait(void* control, PartyPortrait* out)`
- L292 `int PartySelectionChosenCount(void* panel)`
- L298 `bool PartyTableIsNPCAvailable(int npcSlot)`
- L311 `bool PartyTableIsNPCSelectable(int npcSlot)`
- L338 `bool GetPartyNpcNameForSlot(int npcSlot, char* outBuf, size_t bufSize)`
