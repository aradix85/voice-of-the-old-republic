// Descriptions behind an action-bar action id: feats, Force powers and items.

#include "engine_reads_items_internal.h"

namespace acc::engine {

namespace {

// CSWGuiInterfaceAction descriptors encode an action-type tag in the
// high nibble of the action_id at +0x08; the low 28 bits carry the
// category-specific lookup key. Tags decoded via decompile of the
// CSWCCreature entry-creators:
//
//   0x10000000  feat        — CSWCCreature::EnableFeatForMenu      @0x00618a30
//                             ("action_id = feat_id | 0x10000000")
//   0x20000000  force power — CSWCCreatureStats_ClassInfo::GetMenuInfo
//                             @0x0064a870 ("action_id = spell_id | 0x20000000")
//   0x40000000  item        — CSWCCreature::CreateUsableItemEntry  @0x006193a0
//                             ("action_id = server_item.game_object.id | 0x40000000")
//
// Other categories (attack verbs, door open/unlock, computer hack, etc.)
// don't carry a separately addressable description — they're plain verbs
// the engine never surfaces extra text for, so we let the caller fall
// back to the localised "no description" cue.
constexpr uint32_t kActionIdTagMask     = 0xF0000000;
constexpr uint32_t kActionIdTagFeat     = 0x10000000;
constexpr uint32_t kActionIdTagSpell    = 0x20000000;
constexpr uint32_t kActionIdTagItem     = 0x40000000;

typedef void* (__thiscall* PFN_RulesGetFeat)(void* rules, uint16_t featIdx);

typedef void* (__thiscall* PFN_SpellArrayGetSpell)(void* spells, int spellId);

}  // namespace

static void* GetRulesGlobal() {
    void* rules = nullptr;
    __try {
        rules = *reinterpret_cast<void**>(kAddrRulesGlobal);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return rules;
}

bool ResolveFeatDescription(uint32_t featIdx, char* outBuf, size_t bufSize) {
    void* rules = GetRulesGlobal();
    if (!rules) return false;

    void* feat = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_RulesGetFeat>(kAddrCSWRulesGetFeat);
        feat = fn(rules, static_cast<uint16_t>(featIdx));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!feat) return false;

    // Read the description strref off the feat row and resolve it through
    // the dual-game TLK path — mirror of ResolveSpellDescription below.
    // This replaced the CSWFeat::GetDescriptionText call (all it does with
    // the same +0xc strref) because its address was KOTOR 1-only, so on K2
    // the fetch faulted into the SEH and descriptions came back empty.
    // Offset is Same(0x0c), witnessed on K2 in the abilities OnEnterFeat
    // twin.
    uint32_t descStrRef = 0;
    __try {
        descStrRef = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(feat) +
            kFeatDescriptionStrRefOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (descStrRef == 0 || descStrRef == 0xffffffff) return false;
    return LookupTlk(descStrRef, outBuf, bufSize);
}

bool ResolveSpellDescription(uint32_t spellId, char* outBuf, size_t bufSize) {
    void* rules = GetRulesGlobal();
    if (!rules) return false;

    void* spellArray = nullptr;
    __try {
        spellArray = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(rules) + kRulesSpellsOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!spellArray) return false;

    void* spell = nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_SpellArrayGetSpell>(
            kAddrCSWSpellArrayGetSpell);
        spell = fn(spellArray, static_cast<int>(spellId));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!spell) return false;

    uint32_t descStrRef = 0;
    __try {
        descStrRef = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(spell) +
            kSpellDescriptionStrRefOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (descStrRef == 0 || descStrRef == 0xffffffff) return false;
    return LookupTlk(descStrRef, outBuf, bufSize);
}

bool ResolveActionDescriptionFromActionId(uint32_t actionId,
                                          char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    if (actionId == 0 || actionId == 0xffffffff) return false;

    uint32_t tag   = actionId & kActionIdTagMask;
    uint32_t lowId = actionId & ~kActionIdTagMask;
    switch (tag) {
        case kActionIdTagItem: {
            void* item = ResolveItemFromServerHandle(lowId);
            if (!item) return false;
            return ReadItemPropertyDescription(item, outBuf, bufSize);
        }
        case kActionIdTagSpell:
            return ResolveSpellDescription(lowId, outBuf, bufSize);
        case kActionIdTagFeat:
            return ResolveFeatDescription(lowId, outBuf, bufSize);
        default:
            return false;
    }
}

void* ItemFromActionId(uint32_t actionId) {
    if (actionId == 0 || actionId == 0xffffffff) return nullptr;
    if ((actionId & kActionIdTagMask) != kActionIdTagItem) return nullptr;
    return ResolveItemFromServerHandle(actionId & ~kActionIdTagMask);
}

}  // namespace acc::engine
