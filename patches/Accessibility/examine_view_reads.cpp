// Reading a creature for the examine panel: hit points, level, faction, flags, equipment, and the
// names of feats and spells.

#include "examine_view_internal.h"

#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_reads.h"   // LookupTlk — strref → localized text, both games
#include "engine_app.h"     // GetClientApp
#include "engine_player.h"
#include "engine_rebase.h"

namespace acc::examine_view {

namespace {

typedef void* (__thiscall* PFN_GetFeat)(void* rules, unsigned short featIdx);

// Manual-fire engine accessors. Each is deterministic, no-side-effect.
typedef int (__thiscall* PFN_GetIntThis)(void* this_);
typedef int (__thiscall* PFN_GetIntThisInt)(void* this_, int arg);

// Reuse the same LastTarget read path combat_query uses. Local copy to
// avoid header cycle with combat_query.
typedef uint32_t (__thiscall* PFN_GetLastTarget)(void* this_);
const uintptr_t kAddrCClientExoAppGetLastTargetLocal = acc::addr::R(0x005EDD80);

void* GetCSWRules() {
    __try {
        // CSWRules is at offset 0 of CSWSRules — same pointer for both.
        return *reinterpret_cast<void**>(kAddrRulesGlobal);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int CallIntThis(void* this_, uintptr_t addr) {
    if (!this_) return -1;
    __try {
        auto fn = reinterpret_cast<PFN_GetIntThis>(addr);
        return fn(this_);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int CallIntThisInt(void* this_, int arg, uintptr_t addr) {
    if (!this_) return -1;
    __try {
        auto fn = reinterpret_cast<PFN_GetIntThisInt>(addr);
        return fn(this_, arg);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool ReadDeadFlag(void* serverCreature) {
    int v = CallIntThis(serverCreature, kAddrCSWSCreatureGetDead);
    return v != 0 && v != -1;
}

}  // namespace

bool IsSentinel(uint32_t handle) {
    return handle == 0u || handle == 0xFFFFFFFFu || handle == 0x7F000000u;
}

void* ReadCreatureStats(void* serverCreature) {
    if (!serverCreature) return nullptr;
    __try {
        return *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(serverCreature) +
            kCreatureStatsPtrOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int ReadHpMax(void* serverCreature) {
    // GetMaxHitPoints(param_1=1) — includes Toughness / class HP totals.
    int v = CallIntThisInt(serverCreature, 1,
                           kAddrCSWSCreatureGetMaxHitPoints);
    if (v < 0 || v > 0x4000) return -1;
    return v;
}

int ReadLevel(void* serverCreature) {
    void* stats = ReadCreatureStats(serverCreature);
    // GetLevel(0) — raw total class levels (no negative-level subtract).
    // Both games return the level in AL with incidental upper bytes (K1's
    // loop counter, K2's level-scaling float-conversion leftovers), so
    // mask to the low byte before the range check — same trap the
    // damage-level accessor had.
    int v = CallIntThisInt(stats, 0,
                           kAddrCSWSCreatureStatsGetLevel);
    if (v == -1) return -1;
    v &= 0xFF;
    if (v <= 0 || v > 60) return -1;
    return v;
}

int ReadDamageLevel(void* obj) {
    // Shared dual-game reader: K1 engine accessor, K2 ratio replica.
    return acc::engine::ReadObjectDamageLevel(obj);
}

// NOTE: there is intentionally no ReadInvisibleFlag here. The engine's
// CSWSCreature::GetInvisible @0x00501950 is NOT a pure read — its SANCTUARY
// branch rolls a saving throw and mutates perception state (PacifyCreature /
// AddToVisibleList), so it must not be called from this passive readout. And it
// is redundant: AppendEffectRows below already walks the effect list read-only
// and names an active INVISIBILITY effect (type 47 → "Invisible"). Decompile of
// GetInvisible: param_1 is the OBSERVER; it answers "is this creature invisible
// to that observer" (Stealth/Sanctuary, gated by the observer's see-invisibility
// flags + a Will save). See git history for the full analysis.

bool ReadBlindFlag(void* serverCreature) {
    int v = CallIntThis(serverCreature, kAddrCSWSCreatureGetBlind);
    return v != 0 && v != -1;
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
    if (!stats) return -1;
    __try {
        unsigned short raw = *reinterpret_cast<unsigned short*>(
            reinterpret_cast<unsigned char*>(stats) +
            kStatsFactionIdOffset);
        if (raw == 0xFFFF) return -1;
        return static_cast<int>(raw);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
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
    float d = static_cast<float>(std::sqrt(static_cast<double>(DistXYSq(tgt, pc))));
    return static_cast<int>(d + 0.5f);
}

// Read an equipped item's display name by walking the inventory ulong
// handle at the given slot offset (kInventory*HandleOffset). Same path
// combat_query::ReadEquippedItemName uses — duplicated to avoid a
// header cycle between examine_view and combat_query.
bool ReadEquippedItemNameAtSlot(void* serverCreature, size_t slotOffset,
                                char* outBuf, size_t outBufSize) {
    if (!serverCreature || !outBuf || outBufSize < 2) return false;
    outBuf[0] = '\0';
    void* inventory = nullptr;
    __try {
        inventory = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(serverCreature) +
            kCreatureInventoryOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!inventory) return false;

    uint32_t handle = 0;
    __try {
        handle = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(inventory) + slotOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (IsSentinel(handle)) return false;

    if (!acc::engine::GetObjectDisplayNameByHandle(
            handle, outBuf, outBufSize)) {
        return false;
    }
    // Brace-wrapped names are unlocalized dev placeholders on NPC-only
    // creature-prop weapons (see combat_query::ReadEquippedItemName) —
    // treat as an empty slot.
    if (outBuf[0] == '{') {
        outBuf[0] = '\0';
        return false;
    }
    return true;
}

uint32_t ReadLastTargetHandle() {
    void* exoApp = acc::engine::GetClientApp();
    if (!exoApp) return 0;
    __try {
        auto fn = reinterpret_cast<PFN_GetLastTarget>(
            kAddrCClientExoAppGetLastTargetLocal);
        return fn(exoApp);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// Public — true iff serverObject is a Creature whose faction classifies as
// hostile. Reuses the same faction table that drives the examine panel's
// Faction row (ReadFactionId + FactionWordIdFor), so the classification stays
// consistent with what the user hears elsewhere. SEH-guarded via the readers.
// Companions (faction 0/2/4 → Friendly) and neutral townsfolk are excluded;
// a neutral-until-provoked enemy would also read as non-hostile until it turns.
bool IsCreatureDead(void* serverCreature) {
    if (!serverCreature) return false;
    if (acc::engine::GetObjectKind(serverCreature) !=
        static_cast<int>(acc::engine::GameObjectKind::Creature)) {
        return false;
    }
    return ReadDeadFlag(serverCreature);
}

bool IsHostileCreature(void* serverObject) {
    if (!serverObject) return false;
    if (acc::engine::GetObjectKind(serverObject) !=
        static_cast<int>(acc::engine::GameObjectKind::Creature)) {
        return false;
    }
    int faction = ReadFactionId(serverObject);
    if (faction < 0) return false;
    return FactionWordIdFor(faction) == acc::strings::Id::FactionHostile;
}

// Public — same shape as ResolveFeatName below, but for the spells
// array. Reads Rules->spells (CSWSpellArray* at +kRulesSpellsOffset),
// resolves a CSWSpell* via GetSpell(spell_id), then reads the name strref
// off the row and resolves it through the dual-game TLK path (the retired
// CSWSpell::GetSpellNameText call was a bare fetch of the same strref, but
// KOTOR 1-only — on K2 it resolved to 0 and every queued power spoke as the
// generic "Use Force Power"). SEH-guarded at every dereference. Used by
// combat::queue's row speech for action_type=9 (Cast Force Power) entries.
bool ResolveSpellName(int spellId, char* outBuf, size_t outBufSize) {
    if (!outBuf || outBufSize < 2) return false;
    outBuf[0] = '\0';
    void* rules = GetCSWRules();
    if (!rules) return false;

    void* spellArray = nullptr;
    __try {
        spellArray = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(rules) +
            kRulesSpellsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!spellArray) return false;

    using PFN_GetSpell = void* (__thiscall*)(void* spells, int spellId);
    void* spell = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetSpell>(kAddrCSWSpellArrayGetSpell);
        spell = fn(spellArray, spellId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!spell) return false;

    uint32_t nameStrRef = 0;
    __try {
        nameStrRef = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(spell) + kSpellNameStrRefOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (nameStrRef == 0 || nameStrRef == 0xffffffff) return false;
    return acc::engine::LookupTlk(nameStrRef, outBuf, outBufSize);
}

// Public — declared in examine_view.h. Lives outside the anonymous
// namespace so combat::queue (and any other future caller) can link to
// it. Internal helpers (GetCSWRules, PFN_GetFeat,
// kAddrCSWRulesGetFeat) reach through the anon-namespace using-directive
// that the enclosing acc::examine_view namespace implicitly maintains.
// SEH-guarded — feats-table walk.
bool ResolveFeatName(unsigned short featIdx, char* outBuf, size_t outBufSize) {
    if (!outBuf || outBufSize < 2) return false;
    outBuf[0] = '\0';
    void* rules = GetCSWRules();
    if (!rules) return false;

    void* feat = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetFeat>(kAddrCSWRulesGetFeat);
        feat = fn(rules, featIdx);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!feat) return false;

    // Read the name strref off the feat row and resolve it through the
    // dual-game TLK path. This replaced the CSWFeat::GetNameText call
    // (which is all that accessor does with the same +0x8 strref) because
    // its address was KOTOR 1-only — on K2 the call resolved to 0, the SEH
    // swallowed the fault, and every queued feat spoke as the generic
    // "Use talent" (2026-08-04 K2 session). Offset is Same(0x08),
    // witnessed on K2 in the abilities OnEnterFeat twin.
    uint32_t nameStrRef = 0;
    __try {
        nameStrRef = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(feat) + kFeatNameStrRefOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (nameStrRef == 0 || nameStrRef == 0xffffffff) return false;
    return acc::engine::LookupTlk(nameStrRef, outBuf, outBufSize);
}

}  // namespace acc::examine_view
