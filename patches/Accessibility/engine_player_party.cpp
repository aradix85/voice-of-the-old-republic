// Who is in the party, who is leading, and what they are called: leader and
// creature resolution, the party table, solo mode, roster availability and
// the companion names. Declarations are in engine_player.h.

#include "engine_player.h"
#include "engine_player_internal.h"

#include <windows.h>
#include <cstdint>

#include "engine_app.h"     // GetServerAppInternal
#include "engine_area.h"
#include "engine_game.h"    // IsKotor2 — GetPartyCreatureBySlotK2 gate
#include "engine_reads.h"
#include "log.h"
#include "engine_addr.h"
#include "engine_offsets_select.h"

namespace acc::engine {

void* GetPlayerServerCreature() {
    return GetPlayerServerObject();
}

void* GetClientLeader() {
    void* exoApp = GetClientApp();
    if (!exoApp) return nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetPlayerCreature>(
            kAddrGetPlayerCreature);
        return fn(exoApp);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* GetPartyCreatureBySlotK2(int slot) {
    if (!acc::game::IsKotor2()) return nullptr;
    if (slot < 0 || slot > 2) return nullptr;
    // GetPartyCreature @ K2 0x007E5DA0 — the resolver the K2 medical-use
    // handler feeds its picked party slot into (and whose slot-0 wrapper
    // PopulateMenus/DoPersonalAction use as "the player"). Its `this` is the
    // PARTY object at CClientExoAppInternal+0x270 — witnessed in the getter
    // 0x0073FB90 ([app+4]+0x270) that every engine caller runs first.
    typedef void* (__thiscall* PFN_K2GetPartyCreature)(void* this_, int slot);
    void* internal = GetClientAppInternal();
    if (!internal) return nullptr;
    __try {
        void* party = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internal) + 0x270);
        if (!party) return nullptr;
        auto fn = reinterpret_cast<PFN_K2GetPartyCreature>(0x007E5DA0);
        return fn(party, slot);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int SetLeaderQueueModeBit(int on) {
    // CSWCCreature.field200_0x440 — same offset combat_diag reads as `cm`,
    // set by CSWCCreature::SetCombatMode @0x00610a10. Bit 0 is the
    // replace-vs-append discriminator both action dispatchers branch on.
    // K2 0x458 — witnessed in the K2 DoTargetAction twin 0x007445D0.
    const size_t kCSWCCreatureCombatModeOffset = acc::off::Pick(0x440, 0x458);
    void* leader = GetClientLeader();
    if (!leader) return -1;
    __try {
        uint8_t* p = reinterpret_cast<uint8_t*>(leader) +
                     kCSWCCreatureCombatModeOffset;
        uint8_t prev = *p;
        if (on) *p = static_cast<uint8_t>(prev | 0x01u);
        else    *p = static_cast<uint8_t>(prev & ~0x01u);
        return prev & 0x01;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool IsAnyPartyMemberInCombat() {
    // The global combat flag we poll (CClientExoApp::GetCombatMode) only ever
    // mirrors the *controlled* leader's per-creature combat bit — CSWParty::
    // SetLeader re-syncs it on every Tab (call-at 0x0063596d), so switching to
    // a not-yet-engaged member reads as "combat ended" mid-encounter. The bit
    // is in fact maintained for ALL party members every tick by
    // CClientExoAppInternal::UpdateCombatMode @0x005f3ad0, which loops
    // CSWParty::GetCharacter and calls CSWCCreature::SetCombatMode per member.
    // OR that bit across the whole party for the true encounter state. Stays
    // client-side on the same field/namespace as the global we already poll,
    // so there is no client/server timing skew between the two.
    // CClientExoAppInternal.party
    const size_t kInternalPartyOffset = acc::off::Todo(0x270);
    // field200_0x440 bit 0; K2 via the DoTargetAction twin
    const size_t kCSWCCreatureCombatModeOffset = acc::off::Pick(0x440, 0x458);
    // __thiscall(int) -> CSWCCreature*
    const uintptr_t kAddrCSWPartyGetCharacter = acc::addr::R(0x006346C0);
    constexpr int kPartyScanCap = 8;  // a KOTOR party is at most 3
    typedef void* (__thiscall* PFN_GetCharacter)(void* this_, int index);
    void* internal = GetClientAppInternal();
    if (!internal) return false;
    __try {
        void* party = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internal) + kInternalPartyOffset);
        if (!party) return false;
        auto getChar = reinterpret_cast<PFN_GetCharacter>(
            kAddrCSWPartyGetCharacter);
        // GetCharacter is internally bounds-checked (returns null for any
        // index >= party_count), so a fixed cap with a null-skip is safe even
        // though party_member_datas is contiguous.
        for (int i = 0; i < kPartyScanCap; ++i) {
            void* c = getChar(party, i);
            if (!c) continue;
            uint8_t bits = *(reinterpret_cast<uint8_t*>(c) +
                             kCSWCCreatureCombatModeOffset);
            if (bits & 0x01u) return true;
        }
        return false;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

namespace {

typedef int (__thiscall* PFN_PartyTableGetNPCObject)(void*, int, int, int);

// The object id of the creature in a roster slot (an index: 2 = Carth,
// 6 = Mission), 0 when none is live. Like the engine's OnPanelAdded:
// (slot, 0, 1) first, then the second-instance pool (slot, 1, 1).
uint32_t NpcObjectId(void* table, int slot) {
    __try {
        auto fn = reinterpret_cast<PFN_PartyTableGetNPCObject>(
            kAddrCSWPartyTableGetNPCObject);
        int id = fn(table, slot, 0, 1);
        if (id == 0) id = fn(table, slot, 1, 1);
        return static_cast<uint32_t>(id);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// A server creature's stats.first_name, a pure memory read. Companions
// usually have one; the PC's is empty in vanilla saves.
bool StatsFirstName(void* serverCreature, char* outBuf, size_t bufSize) {
    void* stats = nullptr;
    return TryReadPtr(serverCreature, kCreatureStatsPtrOffset, &stats) && stats &&
           ExtractTextOrStrRef(stats, kCreatureStatsFirstNameOffset,
                               kCreatureStatsFirstNameOffset + 4, outBuf, bufSize) &&
           outBuf[0] != '\0';
}

}  // namespace

// The name of the CURRENTLY CONTROLLED leader: Tab re-wires
// CClientExoApp::GetPlayerCreature to the new one.
bool GetActiveLeaderName(char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    void* clientLeader = GetClientLeader();
    void* serverCreature = nullptr;
    TryReadPtr(clientLeader, kClientObjectServerObjectOffset, &serverCreature);
    const uint32_t leaderHandle = clientLeader ? GetObjectHandle(clientLeader) : 0u;

    // Trace, not Write: this is asked dozens of times per tick with the
    // same answer, and Trace folds repeats.
    auto found = [&](const char* via) {
        acclog::Trace("PartyLeader", "leader=%s - client=%p server=%p handle=0x%08x "
                      "name=[%s]", via, clientLeader, serverCreature, leaderHandle,
                      outBuf);
        return true;
    };

    // 1. The engine's display-name accessor on the leader's handle, the one
    //    the sighted UI uses: localized companion names. During the
    //    chargen-to-world transient it overruns its stack buffer on the PC
    //    handle (an uncatchable /GS fastfail); the Tab caller gates that
    //    window with GetPlayerPosition.
    outBuf[0] = '\0';
    if (leaderHandle != 0u &&
        GetObjectDisplayNameByHandle(leaderHandle, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return found("handle");
    }
    // 2. The PC's client handle is the "player-controlled" sentinel, whose
    //    display name is empty. The server creature keeps its identity.
    outBuf[0] = '\0';
    if (serverCreature && GetObjectName(serverCreature, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return found("server-object");
    }
    // 3. stats.first_name.
    outBuf[0] = '\0';
    if (serverCreature && StatsFirstName(serverCreature, outBuf, bufSize)) {
        return found("stats");
    }
    // 4. The chargen-name slot: last, because it can hold a stale name.
    outBuf[0] = '\0';
    if (GetPlayerCharacterName(outBuf, bufSize) && outBuf[0] != '\0') {
        return found("chargen-name");
    }
    acclog::Trace("PartyLeader", "all paths empty - client=%p server=%p handle=0x%08x",
                  clientLeader, serverCreature, leaderHandle);
    return false;
}

bool GetPlayerCharacterName(char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    void* exoApp = GetClientApp();
    if (!exoApp) return false;
    __try {
        auto fn = reinterpret_cast<PFN_GetPlayerCharacterName>(
            kAddrCClientExoAppGetPlayerCharacterName);
        void* exoStr = fn(exoApp);
        if (!exoStr) return false;
        // CExoString layout (c_string @+0, length @+4) — same shape every
        // other reader hits via ReadCExoString.
        return ReadCExoString(exoStr, /*offset=*/0, outBuf, bufSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}


int GetPartyMembers(uint32_t* outHandles, int maxCount) {
    if (!outHandles || maxCount <= 0) return 0;
    void* table = GetServerPartyTable();
    uint32_t numMembers = 0;
    if (!TryReadU32(table, kPartyTableNumMembersOffset, &numMembers) ||
        numMembers == 0 ||
        numMembers > static_cast<uint32_t>(kPartyTableMaxMembers)) {
        return 0;
    }
    // pt_member_ids holds roster slot indices; each resolves to the live
    // creature's object id, comparable to GetObjectHandle. Slots without a
    // live creature are skipped.
    int written = 0;
    for (uint32_t i = 0; i < numMembers && written < maxCount; ++i) {
        uint32_t slot = 0;
        if (!TryReadU32(table, kPartyTableMemberIdsOffset + i * sizeof(int32_t),
                        &slot)) {
            break;
        }
        const uint32_t id = NpcObjectId(table, static_cast<int>(slot));
        if (id != 0) outHandles[written++] = id;
    }
    return written;
}

void* GetServerPartyTable() {
    // The party table is embedded in CServerExoAppInternal (the internal,
    // not the facade). acc::off::Ptr gives nullptr for an unported offset,
    // where plain arithmetic would hand callers a non-null wild pointer.
    void* serverInternal = GetServerAppInternal();
    return acc::off::Ptr(serverInternal, kServerInternalPartyTableOffset);
}

bool GetSoloMode() {
    uint32_t solo = 0;
    return TryReadU32(GetServerPartyTable(), kPartyTableSoloModeOffset, &solo) &&
           solo != 0;
}

namespace {

typedef int  (__thiscall* PFN_PartyTableGetIsNPCAvailable)(void*, int);
typedef char (__thiscall* PFN_PartyTableGetNPCSelectability)(void*, int);

}  // namespace

int PartyRosterSlotCount() {
    return acc::game::IsKotor2() ? 12 : 9;
}

bool IsPartyPortrait(void* control) {
    return acc::engine::HasVtable(control, kVtableCSWGuiPartySelectionButton);
}

bool ReadPartyPortrait(void* control, PartyPortrait* out) {
    if (!out) return false;
    out->npcSlot = -1;
    out->partyId = -1;
    out->selected = 0;
    out->flags = 0;
    if (!IsPartyPortrait(control)) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(control);
        out->selected = *reinterpret_cast<int*>(base + kPartyPortraitSelectedOffset);
        out->flags    = *reinterpret_cast<uint32_t*>(base + kPartyPortraitFlagsOffset);
        out->partyId  = *reinterpret_cast<int*>(base + kPartyPortraitPartyIdOffset);
        out->npcSlot  = *reinterpret_cast<int*>(base + kPartyPortraitNpcSlotOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->npcSlot = -1;
        out->partyId = -1;
        out->selected = 0;
        return false;
    }
    return true;
}

int PartySelectionChosenCount(void* panel) {
    uint32_t count = 0;
    if (!TryReadU32(panel, kPartySelectionChosenCountOffset, &count)) return -1;
    return static_cast<int>(count);
}

bool PartyTableIsNPCAvailable(int npcSlot) {
    if (npcSlot < 0 || npcSlot >= PartyRosterSlotCount()) return false;
    void* table = GetServerPartyTable();
    if (!table) return false;
    __try {
        auto fn = reinterpret_cast<PFN_PartyTableGetIsNPCAvailable>(
            kAddrCSWPartyTableGetIsNPCAvailable);
        return fn(table, npcSlot) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool PartyTableIsNPCSelectable(int npcSlot) {
    if (npcSlot < 0 || npcSlot >= PartyRosterSlotCount()) return false;
    void* table = GetServerPartyTable();
    if (!table) return false;
    __try {
        auto fn = reinterpret_cast<PFN_PartyTableGetNPCSelectability>(
            kAddrCSWPartyTableGetNPCSelectability);
        return fn(table, npcSlot) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// KOTOR 1's roster by slot index, for a companion GetNPCObject cannot
// resolve to a live creature. Proper nouns, the same in every language.
static const char* const kK1CompanionNamesBySlot[9] = {
    "Bastila Shan",
    "Canderous Ordo",
    "Carth Onasi",
    "HK-47",
    "Jolee Bindo",
    "Juhani",
    "Mission Vao",
    "T3-M4",
    "Zaalbar",
};

bool GetPartyNpcNameForSlot(int npcSlot, char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize == 0) return false;
    outBuf[0] = '\0';
    if (npcSlot < 0 || npcSlot >= PartyRosterSlotCount()) return false;
    void* table = GetServerPartyTable();
    const uint32_t handle = table ? NpcObjectId(table, npcSlot) : 0u;
    if (handle != 0) {
        // The client-side display name: companions in the current module.
        if (GetObjectDisplayNameByHandle(handle, outBuf, bufSize) &&
            outBuf[0] != '\0') {
            return true;
        }
        // The client namespace cannot see this handle: on the Endar Spire
        // slot 0 is Trask, a live SERVER object. Name the server creature:
        // first_name where filled, else the universal accessor (for Trask
        // his tag, still the real occupant rather than the table's
        // "Bastila").
        outBuf[0] = '\0';
        if (void* serverObj = ResolveServerObjectHandle(handle)) {
            if (StatsFirstName(serverObj, outBuf, bufSize)) return true;
            outBuf[0] = '\0';
            if (GetObjectName(serverObj, outBuf, bufSize) && outBuf[0] != '\0') {
                return true;
            }
        }
        outBuf[0] = '\0';
    }
    // Not resolved: mostly a companion on the roster but not in the current
    // module. KOTOR 1 falls back to the fixed roster. KOTOR 2 needs no
    // table: its GetNPCObject template-loads a benched companion, so an
    // unresolved slot stays nameless rather than guessed.
    if (acc::game::IsKotor2()) return false;
    const char* fixed = kK1CompanionNamesBySlot[npcSlot];
    const size_t nlen = strlen(fixed);
    if (nlen + 1 > bufSize) return false;
    memcpy(outBuf, fixed, nlen + 1);
    return true;
}

}  // namespace acc::engine
