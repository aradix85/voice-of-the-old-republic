# engine_player_party.cpp (461 lines)

Source: `patches/Accessibility/engine_player_party.cpp`

Who is in the party, who is leading, and what they are called: leader and creature resolution, the party table, solo mode, roster availability and the companion names. Declarations are in engine_player.h.

## Declarations (in source order)

- L21 `void* GetPlayerServerCreature()`
- L25 `void* GetClientLeader()`
- L51 `void* PartyCharacter(int index)`
- L67 `void* GetPartyCreatureBySlotK2(int slot)`
- L73 `int SetLeaderQueueModeBit(int on)`
- L88 `bool IsAnyPartyMemberInCombat()`
- L120 `uint32_t NpcObjectId(void* table, int slot)`: The object id of the creature in a roster slot (an index: 2 = Carth, 6 = Mission), 0 when none is live.
- L134 `bool StatsFirstName(void* serverCreature, char* outBuf, size_t bufSize)`: A server creature's stats.first_name, a pure memory read.
- L146 `bool GetActiveLeaderName(char* outBuf, size_t bufSize)`: The name of the CURRENTLY CONTROLLED leader: Tab re-wires CClientExoApp::GetPlayerCreature to the new one.
- L195 `bool GetPlayerCharacterName(char* outBuf, size_t bufSize)`
- L225 `static bool IsPlayerCharacterId(uint32_t id)`: Player objects have the ids above kInvalidObjectId, handed out from 0x7fffffff down, a new one with every load (0x7fffffff at the start of a session, 0x7ffffff3 a few loads later).
- L232 `static uint32_t PlayerCharacterObjectId()`: The object id of the player character's creature, 0 when it is not in the area.
- L254 `static void* StandInServerCreature()`: The creature of the companion the game has put in the player character's place, nullptr when the player character is itself.
- L272 `void* GetPlayerCharacterServerCreature()`
- L282 `int GetPartyMembers(uint32_t* outHandles, int maxCount)`
- L324 `void* GetServerPartyTable()`
- L332 `bool GetSoloMode()`
- L345 `int PartyRosterSlotCount()`
- L349 `bool IsPartyPortrait(void* control)`
- L353 `bool ReadPartyPortrait(void* control, PartyPortrait* out)`
- L375 `int PartySelectionChosenCount(void* panel)`
- L381 `bool PartyTableIsNPCAvailable(int npcSlot)`
- L394 `bool PartyTableIsNPCSelectable(int npcSlot)`
- L421 `bool GetPartyNpcNameForSlot(int npcSlot, char* outBuf, size_t bufSize)`
