// Reading a creature for the examine panel: hit points, level, faction, flags,
// and the names of feats and spells.

#include "examine_view_internal.h"

#include "engine_app.h"      // GetClientApp
#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_player.h"
#include "engine_reads.h"    // TryRead*, LookupTlk
#include "engine_rebase.h"

#include <windows.h>

#include <cmath>

namespace acc::examine_view {

namespace {

typedef int (__thiscall* PFN_GetIntThis)(void* this_);
typedef int (__thiscall* PFN_GetIntThisInt)(void* this_, int arg);

// CClientExoApp::GetLastTarget, the read combat_query uses too.
typedef uint32_t (__thiscall* PFN_GetLastTarget)(void* this_);
const uintptr_t kAddrCClientExoAppGetLastTargetLocal = acc::addr::R(0x005EDD80);

// The engine's side-effect-free int accessors; -1 when the call faults.
int CallIntThis(void* this_, uintptr_t addr) {
    if (!this_) return -1;
    __try {
        return reinterpret_cast<PFN_GetIntThis>(addr)(this_);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int CallIntThisInt(void* this_, int arg, uintptr_t addr) {
    if (!this_) return -1;
    __try {
        return reinterpret_cast<PFN_GetIntThisInt>(addr)(this_, arg);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool CallFlag(void* serverCreature, uintptr_t addr) {
    const int v = CallIntThis(serverCreature, addr);
    return v != 0 && v != -1;
}

bool IsCreature(void* obj) {
    return obj && acc::engine::GetObjectKind(obj) ==
                      static_cast<int>(acc::engine::GameObjectKind::Creature);
}

}  // namespace

bool IsSentinel(uint32_t handle) {
    return handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId;
}

void* ReadCreatureStats(void* serverCreature) {
    void* stats = nullptr;
    if (serverCreature) {
        acc::engine::TryReadPtr(serverCreature, kCreatureStatsPtrOffset, &stats);
    }
    return stats;
}

int ReadHpMax(void* serverCreature) {
    // GetMaxHitPoints(1): with Toughness and the class totals.
    const int v = CallIntThisInt(serverCreature, 1, kAddrCSWSCreatureGetMaxHitPoints);
    if (v < 0 || v > 0x4000) return -1;
    return v;
}

int ReadLevel(void* serverCreature) {
    // GetLevel(0): the raw total of class levels. Both games return it in AL
    // with leftovers in the upper bytes, hence the mask.
    int v = CallIntThisInt(ReadCreatureStats(serverCreature), 0,
                           kAddrCSWSCreatureStatsGetLevel);
    if (v == -1) return -1;
    v &= 0xFF;
    if (v <= 0 || v > 60) return -1;
    return v;
}

int ReadDamageLevel(void* obj) {
    return acc::engine::ReadObjectDamageLevel(obj);
}

// No invisible flag on purpose: CSWSCreature::GetInvisible is not a pure read
// (its Sanctuary branch rolls a save and changes perception), and the effect
// rows already name an active invisibility read-only.
bool ReadBlindFlag(void* serverCreature) {
    return CallFlag(serverCreature, kAddrCSWSCreatureGetBlind);
}

acc::strings::Id DamageLevelStringId(int level) {
    using S = acc::strings::Id;
    switch (level) {
        case 0:  return S::DamageLevel0Healthy;
        case 1:  return S::DamageLevel1Light;
        case 2:  return S::DamageLevel2Wounded;
        case 3:  return S::DamageLevel3Badly;
        case 4:  return S::DamageLevel4Dying;
        case 5:  return S::DamageLevel5Dead;
        default: return S::DamageLevel0Healthy;
    }
}

int ReadFactionId(void* serverCreature) {
    void* stats = ReadCreatureStats(serverCreature);
    uint16_t raw = 0;
    if (!stats || !acc::engine::TryReadU16(stats, kStatsFactionIdOffset, &raw)) return -1;
    return raw == 0xFFFF ? -1 : static_cast<int>(raw);
}

acc::strings::Id FactionWordIdFor(int factionId) {
    using S = acc::strings::Id;
    switch (factionId) {
        case 0:   return S::FactionFriendly;       // PLAYER
        case 1:   return S::FactionHostile;        // HOSTILE_1
        case 2:   return S::FactionFriendly;       // FRIENDLY_1
        case 3:   return S::FactionHostile;        // HOSTILE_2
        case 4:   return S::FactionFriendly;       // FRIENDLY_2
        case 5:   return S::FactionNeutral;        // NEUTRAL
        case 6:   return S::FactionHostile;        // INSANE
        case 7:   return S::FactionHostile;        // PTAT_TUSKAN
        case 8:   return S::FactionHostile;        // GLB_XOR
        case 9:                                    // SURRENDER_1
        case 10:  return S::FactionNeutral;        // SURRENDER_2
        case 11:  return S::FactionHostile;        // PREDATOR
        case 12:  return S::FactionNeutral;        // PREY
        case 13:  return S::FactionHostile;        // TRAP
        case 15:  return S::FactionHostile;        // RANCOR
        case 16:                                   // GIZKA_1
        case 17:  return S::FactionNeutral;        // GIZKA_2
        default:  return S::FactionNeutral;
    }
}

int Read2DDistanceMeters(void* obj) {
    if (!obj) return -1;
    Vector tgt{};
    if (!acc::engine::GetObjectPosition(obj, tgt)) return -1;
    Vector pc{};
    if (!acc::engine::GetPlayerPosition(pc)) return -1;
    const float d = static_cast<float>(std::sqrt(static_cast<double>(DistXYSq(tgt, pc))));
    return static_cast<int>(d + 0.5f);
}

uint32_t ReadLastTargetHandle() {
    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) return 0;
    __try {
        return reinterpret_cast<PFN_GetLastTarget>(kAddrCClientExoAppGetLastTargetLocal)(exoApp);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

bool IsCreatureDead(void* serverCreature) {
    return IsCreature(serverCreature) && CallFlag(serverCreature, kAddrCSWSCreatureGetDead);
}

// The faction table of the examine panel's Faction row, so "hostile" means
// the same everywhere. Companions and neutral townsfolk are not hostile; an
// enemy that is neutral until provoked reads as such until it turns.
bool IsHostileCreature(void* serverObject) {
    if (!IsCreature(serverObject)) return false;
    const int faction = ReadFactionId(serverObject);
    return faction >= 0 && FactionWordIdFor(faction) == acc::strings::Id::FactionHostile;
}

bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize) {
    if (!outBuf || outBufSize < 2) return false;
    outBuf[0] = '\0';
    return acc::engine::ReadRowStrRefText(acc::engine::GetSpellRow(spellId),
                                          kSpellNameStrRefOffset, outBuf, outBufSize);
}

bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize) {
    if (!outBuf || outBufSize < 2) return false;
    outBuf[0] = '\0';
    return acc::engine::ReadRowStrRefText(acc::engine::GetFeatRow(featIdx),
                                          kFeatNameStrRefOffset, outBuf, outBufSize);
}

}  // namespace acc::examine_view
