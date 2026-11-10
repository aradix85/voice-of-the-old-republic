// The rules rows of feats and Force powers, and the descriptions behind an
// action-bar action id: feats, Force powers and items.

#include "engine_reads_items_internal.h"

namespace acc::engine {

namespace {

// A CSWGuiInterfaceAction's action_id carries a type tag in its high nibble
// and the lookup key in the low 28 bits (from the CSWCCreature entry
// creators: EnableFeatForMenu 0x00618a30, ClassInfo::GetMenuInfo 0x0064a870,
// CreateUsableItemEntry 0x006193a0). Other actions are plain verbs without a
// description of their own.
constexpr uint32_t kActionIdTagMask  = 0xF0000000;
constexpr uint32_t kActionIdTagFeat  = 0x10000000;
constexpr uint32_t kActionIdTagSpell = 0x20000000;
constexpr uint32_t kActionIdTagItem  = 0x40000000;

typedef void* (__thiscall* PFN_RulesGetFeat)(void* rules, uint16_t featIdx);
typedef void* (__thiscall* PFN_SpellArrayGetSpell)(void* spells, int spellId);

void* GetRulesGlobal() {
    void* rules = nullptr;
    TryReadPtr(reinterpret_cast<void*>(kAddrRulesGlobal), 0, &rules);
    return rules;
}

}  // namespace

void* GetFeatRow(uint16_t featIdx) {
    void* rules = GetRulesGlobal();
    if (!rules) return nullptr;
    __try {
        return reinterpret_cast<PFN_RulesGetFeat>(kAddrCSWRulesGetFeat)(rules, featIdx);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* GetSpellRow(int spellId) {
    void* rules = GetRulesGlobal();
    void* spellArray = nullptr;
    if (!rules || !TryReadPtr(rules, kRulesSpellsOffset, &spellArray) || !spellArray) {
        return nullptr;
    }
    __try {
        return reinterpret_cast<PFN_SpellArrayGetSpell>(kAddrCSWSpellArrayGetSpell)(
            spellArray, spellId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// Through the dual-game TLK path: the engine's own Get*Text accessors were
// KOTOR 1-only addresses.
bool ReadRowStrRefText(void* row, size_t offset, char* outBuf, size_t bufSize) {
    uint32_t strRef = 0;
    if (!row || !TryReadU32(row, offset, &strRef)) return false;
    if (strRef == 0 || strRef == 0xffffffff) return false;
    return LookupTlk(strRef, outBuf, bufSize);
}

namespace {

bool ResolveFeatDescription(uint32_t featIdx, char* outBuf, size_t bufSize) {
    return ReadRowStrRefText(GetFeatRow(static_cast<uint16_t>(featIdx)),
                             kFeatDescriptionStrRefOffset, outBuf, bufSize);
}

bool ResolveSpellDescription(uint32_t spellId, char* outBuf, size_t bufSize) {
    return ReadRowStrRefText(GetSpellRow(static_cast<int>(spellId)),
                             kSpellDescriptionStrRefOffset, outBuf, bufSize);
}

}  // namespace

bool ResolveActionDescriptionFromActionId(uint32_t actionId,
                                          char* outBuf, size_t bufSize) {
    if (!outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    if (actionId == 0 || actionId == 0xffffffff) return false;

    const uint32_t lowId = actionId & ~kActionIdTagMask;
    switch (actionId & kActionIdTagMask) {
        case kActionIdTagItem: {
            void* item = ResolveItemFromServerHandle(lowId);
            return item && ReadItemPropertyDescription(item, outBuf, bufSize);
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
