// CSWSCombatRound::AddAction detour: the trigger of the combat-queue announce
// ("X, position N" / "X, queue full"). Every genuine engine action-add is
// forwarded to acc::combat::queue::OnEngineActionAdded. The log-only
// CSWSCombatRound hooks are in combat_diag.cpp.

#include "combat_diag.h"

#include <cstdint>

#include "combat_queue.h"
#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_reads.h"
#include "log.h"

extern "C" void __cdecl OnCombatRoundAddAction(void* this_combatRound,
                                               void* esp_action_addr,
                                               void* esp_param2_addr) {
    // A `source = "esp+N"` parameter arrives as the ADDRESS of the stack slot
    // (the wrapper emits LEA), so both are dereferenced here.
    void* action = nullptr;
    uint32_t param2 = 0;
    acc::engine::TryReadPtr(esp_action_addr, 0, &action);
    acc::engine::TryReadU32(esp_param2_addr, 0, &param2);

    // 1 attack, 6 equip, 7 unequip, 9 cast force, 10 use item, 11 use feat.
    uint16_t typeSlot = 0;
    const unsigned actionType =
        acc::engine::TryReadU16(action, kCombatRoundActionTypeOffset, &typeSlot)
            ? (typeSlot & 0xffu) : 0xffu;

    acclog::Write("Combat.Diag",
        "ADD   [%s] round=%p action=%p type=%u param2=%d",
        acc::combat::queue::RoleTag(this_combatRound),
        this_combatRound, action, actionType, static_cast<int>(param2));

    // The hook fires once per genuine add, at function entry, so the announce
    // never under-counts on key auto-repeat and never races the queue drain.
    // OnEngineActionAdded filters to the controlled creature's round itself.
    acc::combat::queue::OnEngineActionAdded(this_combatRound, action);
}

// CSWSCombatRound::AddAction @0x00590270 on KOTOR 2 (K1 hooks 0x004D3660),
// hooked at 0x00590276 — the `this` store after the prologue. Identity:
// the body checks the queue cap (`GetCount; cmp eax,4`), inserts sorted by
// priority, and frees the action on the full-queue path — K1's structure
// line for line. Unoptimised frame: ECX = this, action* at [EBP+8],
// param2 at [EBP+0xC]; the shared handler wants the SLOT ADDRESSES.
extern "C" __declspec(dllexport)
void __cdecl OnCombatRoundAddActionK2(void* thisRound, void* ebp) {
    if (!acc::game::IsKotor2()) return;
    if (!ebp) return;
    OnCombatRoundAddAction(thisRound,
                           static_cast<char*>(ebp) + 8,
                           static_cast<char*>(ebp) + 0xC);
}
