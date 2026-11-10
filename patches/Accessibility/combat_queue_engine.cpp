// Reading the engine's combat rounds: the queued actions, their kind and target, and the label a
// queued action is spoken by.

#include "combat_queue_internal.h"

#include "engine_area.h"      // ResolveServerObjectHandle, GetObjectName,
                              // GetObjectDisplayNameByHandle
#include "engine_offsets.h"
#include "engine_player.h"    // GetPlayerServerCreature, GetPartyMembers,
#include "examine_view.h"     // ResolveFeatName — decode type=11 entries to
                              // their specific feat name (e.g. "Power
                              // Attack") instead of the generic
                              // "Use talent" verb

namespace acc::combat::queue {

namespace {

// CSWSCombatRound.actions is a CExoLinkedList<CSWSCombatRoundAction>*
// at +0x9b0. The list is a node-chain via `next` pointers; each node
// carries a CSWSCombatRoundAction* in its data slot. Walk to a fixed
// cap (we never expect more than ~16 queued actions in practice).
//
// Filter (validated 2026-05-10): the engine consistently has a leading
// node with action_type=255 (0xFF) and target=0 — appears in every
// queue dump regardless of creature. Likely the "current dispatching
// action" placeholder slot the engine maintains. Skipping these in
// both Count and GetQueueAction surfaces only the real queued entries
// to the user (matches what the sighted in-game UI shows).
constexpr int kMaxQueueWalk = 64;

bool ReadNodeActionType(void* node, unsigned char& outType) {
    if (!node) return false;
    __try {
        void* data = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(node) +
            kLinkedListNodeDataOff);
        if (!data) return false;
        outType = *(reinterpret_cast<unsigned char*>(data) +
                    kCombatRoundActionTypeOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Read the feat_id stored on a type=11 (Use Feat) action node.
// CSWSCombatRoundAction.field25_0x5c is a ushort that CSWGuiMainInterface::
// GetActionIcon @0x686fb0 feeds straight into CSWRules::GetFeat for the
// strip-icon lookup. 0xFFFF sentinel on read fault / non-feat slot.
unsigned short ReadActionFeatId(void* action) {
    if (!action) return 0xffff;
    __try {
        return *reinterpret_cast<unsigned short*>(
            reinterpret_cast<unsigned char*>(action) +
            kCombatRoundActionFeatIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0xffff;
    }
}

// Read the spell_id stored on a type=9 (Cast Force Power) action node.
// CSWSCombatRoundAction.field13_0x24 is an int that GetActionIcon feeds
// into CSWSpellArray::GetSpell. -1 sentinel on read fault / non-spell slot.
int ReadActionSpellId(void* action) {
    if (!action) return -1;
    __try {
        return *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(action) +
            kCombatRoundActionSpellIdOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// Read the item handle stored on a type=10 (Use Item) action node.
// CSWSCombatRoundAction.field27_0x64 is a ulong handle (server-side).
// 0x7F000000 sentinel on read fault / non-item slot.
uint32_t ReadActionItemHandle(void* action) {
    if (!action) return 0x7F000000u;
    __try {
        return *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(action) +
            kCombatRoundActionItemHandleOff);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0x7F000000u;
    }
}

}  // namespace

// Read CSWSCreature.combat_round @+0x9c8.
void* ReadCombatRound(void* serverCreature) {
    if (!serverCreature) return nullptr;
    __try {
        return *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(serverCreature) +
            kCreatureCombatRoundOffset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int CountQueueEntries(void* combatRound) {
    if (!combatRound) return 0;
    int count = 0;
    __try {
        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(combatRound) +
            kCombatRoundActionsOffset);
        if (!listPtr) return 0;
        // listPtr is CExoLinkedList<T>*. Three derefs to reach the head
        // node: list -> internal -> head. Then walk via Node.next at +4.
        void* internalPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(listPtr) +
            kListInternalOffset);
        if (!internalPtr) return 0;
        void* node = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internalPtr) +
            kListInternalHeadOffset);
        int walked = 0;
        while (node && walked < kMaxQueueWalk) {
            ++walked;
            unsigned char t = 0;
            if (ReadNodeActionType(node, t) && t != 0xFF) {
                ++count;
            }
            node = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) +
                kLinkedListNodeNextOff);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return count;
}

// Get the action node at zero-based `index` (skipping placeholder
// type=0xFF entries), or nullptr if out of range.
void* GetQueueAction(void* combatRound, int index) {
    if (!combatRound || index < 0) return nullptr;
    __try {
        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(combatRound) +
            kCombatRoundActionsOffset);
        if (!listPtr) return nullptr;
        void* internalPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(listPtr) +
            kListInternalOffset);
        if (!internalPtr) return nullptr;
        void* node = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(internalPtr) +
            kListInternalHeadOffset);
        int matched = 0, walked = 0;
        while (node && walked < kMaxQueueWalk) {
            ++walked;
            unsigned char t = 0;
            if (ReadNodeActionType(node, t) && t != 0xFF) {
                if (matched == index) {
                    return *reinterpret_cast<void**>(
                        reinterpret_cast<unsigned char*>(node) +
                        kLinkedListNodeDataOff);
                }
                ++matched;
            }
            node = *reinterpret_cast<void**>(
                reinterpret_cast<unsigned char*>(node) +
                kLinkedListNodeNextOff);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return nullptr;
}

// Map an action_type byte to a localised verb.
//
// Enum confirmed 2026-05-14 by decompiling CSWGuiMainInterface::GetActionIcon
// @0x686fb0 — the engine's own per-slot icon resolver. The switch in
// case 0xc (the standard combat-round path the strip uses) decodes:
//
//   action_type=1   → i_attack  / i_attackm   ("Bash door" is just an
//                                              attack with a door target)
//   action_type=6   → i_equip   / i_equipm
//   action_type=7   → i_unequip / i_equipm
//   action_type=9   → SpellArray lookup / i_powerm  (Cast Force Power)
//   action_type=10  → ItemArray lookup / i_useitemm (Use Item)
//   action_type=11  → FeatArray lookup / i_featm    (Use Feat — includes
//                                                    the player's
//                                                    Power Attack /
//                                                    Flurry / etc.)
//
// QueueVerbUseFeat serves as our "Use Feat" word since the existing
// table already groups feat activations under that ID and the word
// reads correctly for both feats and force-power adjacent feats.
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

// Read action_type byte + target handle from a CSWSCombatRoundAction.
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

// Build the spoken label for a queued action node: the SPECIFIC ability name
// for force powers / items / feats (type 9/10/11), else the generic verb
// ("Attack" / "Use Feat" / ...). Shared by SpeakRow (Shift+H review)
// and OnEngineActionAdded (the live queue announce) so both name a queued
// entry identically.
//   type=9  (Force power) — spell_id @+0x24 → CSWSpell::GetSpellNameText
//   type=10 (Item use)    — handle   @+0x64 → engine display-name accessor
//   type=11 (Use feat)    — feat_id  @+0x5c → CSWFeat::GetNameText
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
        if (itemHandle != 0u && itemHandle != 0x7F000000u &&
            acc::engine::GetObjectDisplayNameByHandle(
                itemHandle, abilityName, sizeof(abilityName)) &&
            abilityName[0] != '\0') {
            verb = abilityName;
        }
    }
    std::strncpy(out, verb, n - 1);
    out[n - 1] = '\0';
}

// Read the raw queue count from a combat round (engine's own
// actions->internal->count — the value AddAction's `if (3 < count)` cap
// inspects). -1 on read fault. Same field CountPlayerEntries reads, but on
// an arbitrary round (the one the AddAction detour hands us).
int RawRoundCount(void* combatRound) {
    if (!combatRound) return -1;
    __try {
        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(combatRound) +
            kCombatRoundActionsOffset);
        if (!listPtr) return -1;
        void* internalPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(listPtr) +
            kListInternalOffset);
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

int CountPlayerEntries() {
    // Read the engine's authoritative count field at
    // CSWSCombatRound.actions->internal->count. This matches the value
    // CSWSCombatRound::AddAction's cap check inspects (`if (3 < count)`)
    // so "position N" lines up 1:1 with what the engine considers slot N
    // and "queue full" fires the same press the engine would
    // free the node.
    //
    // We deliberately bypass the 0xFF-placeholder filter that
    // CountQueueEntries (used by the Shift+H row walk) applies. The
    // placeholder slot is part of the engine's queue from the cap-
    // mechanic's point of view; filtering it produces off-by-one
    // announcements where the first press in a fresh combat reads as
    // "used" (filtered count still 0 while the engine already
    // sees count 1).
    void* server = acc::engine::GetPlayerServerCreature();
    if (!server) return 0;
    void* round = ReadCombatRound(server);
    if (!round) return 0;
    __try {
        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(round) +
            kCombatRoundActionsOffset);
        if (!listPtr) return 0;
        void* internalPtr = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(listPtr) +
            kListInternalOffset);
        if (!internalPtr) return 0;
        int n = *reinterpret_cast<int*>(
            reinterpret_cast<unsigned char*>(internalPtr) +
            kListInternalCountOffset);
        if (n < 0 || n > 64) return 0;
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

}  // namespace acc::combat::queue
