# engine_player.h (365 lines)

Source: `patches/Accessibility/engine_player.h`

Player state readers — position, facing, area. SEH-guarded.

Chain: *kAddrAppManagerPtr → AppManager → +0x4 CClientExoApp → CClientExoApp::GetPlayerCreature @0x5ed540 → CSWCCreature* → server_object @+0xf8 → CSWSObject* → position @+0x90, orientation @+0x9c (z=0 for facing — engine 2D unit vector, not quaternion).

CSWSObject::GetArea @0x4cb120 → CSWSArea*.

The +0x4 between AppManager and CClientExoApp wasn't in Q1's prose but every caller of GetPlayerCreature uses it (verified at 0x5fba8d, 0x60541a, 0x605451). Server layout (+0x90/+0x9c) is the authoritative source — client layout (+0x24/+0x30) is a parallel cache.

All functions return false/nullptr cleanly on null at any chain link or SEH fault.

## Declarations (in source order)

- L31 `bool GetPlayerPosition(Vector& out)`
- L34 `bool GetPlayerFacing(Vector& out)`: 2D unit vector — engine zeroes z on object facing.
- L38 `bool GetPlayerYawDegrees(float& out)`: 0° = +X = east, CCW positive, [0, 360).
- L41 `void* GetPlayerArea()`: Opaque CSWSArea*.
- L50 `bool GetCameraPosition(Vector& out)`: World position of the gameplay camera.
- L61 `bool GetCameraYawRadians(float& outRad)`: Camera heading in the engine yaw frame (radians, 0=+X/East, CCW+), read from the module camera's orientation quaternion at Camera+0x88 (Gob+0x84).
- L65 `void* GetPlayerServerCreature()`: Opaque CSWSCreature* for callers that need to thiscall on the player creature (e.g.
- L78 `bool GetPlayerCharacterName(char* outBuf, size_t bufSize)`: Reads CClientExoAppInternal::player_character_name (+0x294) via CClientExoApp::GetPlayerCharacterName @0x5edab0.
- L82 `void* GetClientLeader()`: Active LEADER (Tab cycles which party member is leader).
- L89 `void* GetPartyCreatureBySlotK2(int slot)`: KOTOR 2 only: the client creature in party slot `slot` (0..2) — the slot space the engine's medical-item target-pick consumes (its handler resolves the picked slot through this same accessor; slot 0 = the controlled l...
- L95 `void* ClientToServerCreature(void* clientCreature)`: CSWCCreature* → its CSWSCreature*, game-appropriately: KOTOR 1 reads the tested server_object field, KOTOR 2 calls the engine's own CSWCCreature::GetServerCreature (the client layout shifted there — same dual path Get...
- L111 `int SetLeaderQueueModeBit(int on)`: Active leader's action-queue "append" knob — CSWCCreature.field200_0x440 bit 0 (the combat-mode bit combat_diag reads as `cm`).
- L120 `bool IsAnyPartyMemberInCombat()`: True when ANY party member's per-creature combat bit (CSWCCreature field200_0x440 bit 0) is set — the real "is the encounter active" signal.
- L136 `bool GetActiveLeaderName(char* outBuf, size_t bufSize)`: Three resolution paths, in order: 1.
- L153 `bool SetPlayerInputEnabled(bool enabled, bool armAutoRestore = true)`: Wraps CSWPlayerControl::SetEnabled @0x006792e0.
- L157 `void TickPlayerInputRestore()`: Restore tick — queue-drain / ceiling driven.
- L161 `void TickActionQueueDiag()`: Diagnostic tick — logs player action-queue depth changes (delta only).
- L165 `int GetPlayerActionQueueDepth()`: Player's pending AI-action count (CSWSObject.action_nodes).
- L177 `int GetPartyMembers(uint32_t* outHandles, int maxCount)`: Walks CServerExoApp.party_table @+0x1b770 (via GetServerPartyTable): +0x0 pt_num_members (active followers, 0..2 in normal play — the PC is the implicit leader and is NOT counted here) +0x4 pt_member_ids[] (NPC *roste...
- L181 `void* GetServerPartyTable()`: Active CSWPartyTable*.
- L186 `bool GetSoloMode()`: CSWPartyTable.pt_solomode @+0x190 — the flag the engine's own ExecuteCommandGetSoloMode @0x00546af0 returns and SetSoloMode @0x00565500 writes (both decompiled 2026-07-16).
- L190 `bool PartyTableIsNPCAvailable(int npcSlot)`: CSWPartyTable::GetIsNPCAvailable @0x005636B0.
- L194 `bool PartyTableIsNPCSelectable(int npcSlot)`: CSWPartyTable::GetNPCSelectability @0x005637C0.
- L199 `int PartyRosterSlotCount()`: Roster size the party-select screen renders: 9 on KOTOR 1, 12 on KOTOR 2 (its CSWGuiPartySelection ctor builds 12 party_data elements, and GetIsNPCAvailable bounds-checks `< 0xc`).
- L203 `struct PartyPortrait`: One party-select roster portrait (CSWGuiPartySelectionButton), read through the per-game member offsets in this header.
- L212 `bool IsPartyPortrait(void* control)`: True iff `control` is a party-select portrait.
- L216 `bool ReadPartyPortrait(void* control, PartyPortrait* out)`: Reads a portrait's fields.
- L220 `int PartySelectionChosenCount(void* panel)`: The panel's count of companions currently chosen (the engine refuses an add once it reaches 2).
- L233 `bool GetPartyNpcNameForSlot(int npcSlot, char* outBuf, size_t bufSize)`: Resolves a PartySelection roster slot to a display name.

Also 22 constants or variables at namespace scope.
