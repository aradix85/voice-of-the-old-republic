// Reading the engine's combat rounds: the queued actions, their kind and
// target, and the label a queued action is spoken by.

#include "combat_queue_internal.h"

#include "engine_area.h"
#include "engine_offsets.h"
#include "engine_player.h"
#include "engine_reads.h"
#include "examine_view.h"

namespace acc::combat::queue {

namespace {

// CSWSCombatRound.actions is a CExoLinkedList<CSWSCombatRoundAction>*: list
// -> internal -> head node, then node.next; each node's data slot holds the
// action. Every queue carries a leading placeholder node of type 0xFF and
// target 0, which the in-game UI does not show either, so the walk skips it.
// Capped well above the handful ever queued.
constexpr int kMaxQueueWalk = 64;
constexpr unsigned char kPlaceholderType = 0xFF;

void* NodeAction(void* node) {
    return *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(node) + kLinkedListNodeDataOff);
}

void* NodeNext(void* node) {
    return *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(node) + kLinkedListNodeNextOff);
}

// The node's action type, under its own SEH so one bad node is skipped
// rather than ending the walk.
bool ReadNodeActionType(void* node, unsigned char& outType) {
    if (!node) return false;
    __try {
        void* data = NodeAction(node);
        if (!data) return false;
        outType = *(reinterpret_cast<unsigned char*>(data) +
                    kCombatRoundActionTypeOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The list's internal block, or nullptr. Raw reads; the caller holds the SEH
// frame.
void* ListInternal(void* combatRound) {
    void* listPtr = *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(combatRound) + kCombatRoundActionsOffset);
    if (!listPtr) return nullptr;
    return *reinterpret_cast<void**>(
        reinterpret_cast<unsigned char*>(listPtr) + kListInternalOffset);
}

// Walks the real (non-placeholder) entries. Returns the action of entry
// `index` (none when index < 0); *outCount, if given, gets the number of
// real entries, or 0 on a fault.
void* WalkQueue(void* combatRound, int index, int* outCount) {
    if (outCount) *outCount = 0;
    if (!combatRound) return nullptr;
    int count = 0;
    __try {
        void* internalPtr = ListInternal(combatRound);
        if (!internalPtr) return nullptr;
        void* node = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internalPtr) + kListInternalHeadOffset);
        for (int walked = 0; node && walked < kMaxQueueWalk; ++walked) {
            unsigned char type = 0;
            if (ReadNodeActionType(node, type) && type != kPlaceholderType) {
                if (count == index) return NodeAction(node);
                ++count;
            }
            node = NodeNext(node);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    if (outCount) *outCount = count;
    return nullptr;
}

// The ushort feat id of a type-11 (Use Feat) action, which
// CSWGuiMainInterface::GetActionIcon feeds to CSWRules::GetFeat; 0xFFFF on
// a fault.
unsigned short ReadActionFeatId(void* action) {
    uint16_t v = 0;
    return acc::engine::TryReadU16(action, kCombatRoundActionFeatIdOffset, &v) ? v
                                                                            : 0xffff;
}

// The spell id of a type-9 (Cast Force Power) action, which GetActionIcon
// feeds to CSWSpellArray::GetSpell; -1 on a fault.
int ReadActionSpellId(void* action) {
    uint32_t v = 0;
    return acc::engine::TryReadU32(action, kCombatRoundActionSpellIdOffset, &v)
               ? static_cast<int>(v) : -1;
}

// The server item handle of a type-10 (Use Item) action.
uint32_t ReadActionItemHandle(void* action) {
    uint32_t v = 0;
    return acc::engine::TryReadU32(action, kCombatRoundActionItemHandleOff, &v)
               ? v : kInvalidObjectId;
}

}  // namespace

void* ReadCombatRound(void* serverCreature) {
    void* round = nullptr;
    return acc::engine::TryReadPtr(serverCreature, kCreatureCombatRoundOffset, &round)
               ? round : nullptr;
}

int CountQueueEntries(void* combatRound) {
    int count = 0;
    WalkQueue(combatRound, -1, &count);
    return count;
}

void* GetQueueAction(void* combatRound, int index) {
    if (index < 0) return nullptr;
    return WalkQueue(combatRound, index, nullptr);
}

// The action types decoded by CSWGuiMainInterface::GetActionIcon (0x686fb0),
// the engine's own per-slot icon resolver: 1 attack (a door bash is an
// attack on a door), 6 equip, 7 unequip, 9 cast Force power, 10 use item,
// 11 use feat.
acc::strings::Id VerbForActionType(unsigned char actionType) {
    switch (actionType) {
        case 1:  return acc::strings::Id::QueueVerbAttack;
        case 6:  return acc::strings::Id::QueueVerbEquip;
        case 7:  return acc::strings::Id::QueueVerbUnequip;
        case 9:  return acc::strings::Id::QueueVerbCastForce;
        case 10: return acc::strings::Id::QueueVerbItemCast;
        case 11: return acc::strings::Id::QueueVerbUseFeat;
        default: return acc::strings::Id::QueueVerbUnknown;
    }
}

bool ReadActionFields(void* action, unsigned char& outType,
                      uint32_t& outTarget) {
    outType = 0xff;
    outTarget = 0;
    if (!action) return false;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(action);
        outType   = *(base + kCombatRoundActionTypeOffset);
        outTarget = *reinterpret_cast<uint32_t*>(
            base + kCombatRoundActionTargetOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outType = 0xff;
        outTarget = 0;
        return false;
    }
}

// The specific Force power, item or feat name for types 9, 10 and 11, else
// the generic verb.
void BuildActionLabel(void* action, char* out, size_t n) {
    if (!out || n == 0) return;
    out[0] = '\0';
    unsigned char type = 0xff;
    uint32_t target = 0;
    ReadActionFields(action, type, target);

    char abilityName[96] = "";
    const char* verb = acc::strings::Get(VerbForActionType(type));
    if (type == 11) {
        unsigned short featId = ReadActionFeatId(action);
        if (featId != 0xffff &&
            acc::examine_view::ResolveFeatName(
                featId, abilityName, sizeof(abilityName)) &&
            abilityName[0] != '\0') {
            verb = abilityName;
        }
    } else if (type == 9) {
        int spellId = ReadActionSpellId(action);
        if (spellId >= 0 &&
            acc::examine_view::ResolveSpellName(
                spellId, abilityName, sizeof(abilityName)) &&
            abilityName[0] != '\0') {
            verb = abilityName;
        }
    } else if (type == 10) {
        uint32_t itemHandle = ReadActionItemHandle(action);
        if (itemHandle != 0u && itemHandle != kInvalidObjectId &&
            acc::engine::GetObjectDisplayNameByHandle(
                itemHandle, abilityName, sizeof(abilityName)) &&
            abilityName[0] != '\0') {
            verb = abilityName;
        }
    }
    strncpy_s(out, n, verb, _TRUNCATE);
}

int RawRoundCount(void* combatRound) {
    if (!combatRound) return -1;
    __try {
        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(combatRound) + kCombatRoundActionsOffset);
        if (!listPtr) return -1;
        void* internalPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(listPtr) + kListInternalOffset);
        if (!internalPtr) return 0;
        int n = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(internalPtr) +
            kListInternalCountOffset);
        if (n < 0 || n > 64) return -1;
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// The engine's own count, placeholder included: the value AddAction's cap
// check (`if (3 < count)`) inspects, so "position N" and "queue full" match
// the engine's slots. Filtering the placeholder made the first press of a
// fresh combat read one short.
int CountPlayerEntries() {
    void* server = acc::engine::GetPlayerServerCreature();
    if (!server) return 0;
    int n = RawRoundCount(ReadCombatRound(server));
    return n < 0 ? 0 : n;
}

}  // namespace acc::combat::queue
