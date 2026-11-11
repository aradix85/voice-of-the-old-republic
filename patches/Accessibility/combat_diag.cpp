#include "combat_diag.h"

#include <windows.h>
#include <cstdint>

#include "combat_queue.h"    // queue depths, RoleTag, CancelNoOpWatch
#include "engine_app.h"      // GetClientApp
#include "engine_game.h"
#include "engine_offsets.h"
#include "engine_offsets_select.h"
#include "engine_panels.h"   // ResolveMainInterface
#include "engine_player.h"   // GetPlayerServerCreature, GetClientLeader
#include "engine_reads.h"
#include "log.h"

namespace acc::combat_diag {

namespace {

// Engine accessors used by DoPersonalAction's chain branch.
// GetAutoPaused: K2 facade 0x00740960 is K1's inlined body byte for byte
// ([internal+0x384] & 1) and shows up in the K2 DoPersonalAction call
// census — a double witness.
// GetPauseState: K2 facade 0x0073F680 → internal 0x00784420, which reads
// the SAME [internal+0x1cc] & bit as K1's 0x005EE8F0.
const uintptr_t kAddrCClientExoAppGetAutoPaused = acc::addr::Pick(0x005edef0, 0x00740960);
const uintptr_t kAddrCClientExoAppGetPauseState = acc::addr::Pick(0x005ed640, 0x0073F680);

// main_interface.field1_0x64 — the engine target handle SetTarget stamps.
// K2 0x68 — witnessed by the CSWGuiMainInterface::SetTarget twin 0x0074CFB0,
// which stamps [this+0x68].
const size_t kMainInterfaceTargetHandleOff = acc::off::Pick(0x64, 0x68);

typedef unsigned long (__thiscall* PFN_GetAutoPaused)(void* this_);
typedef unsigned long (__thiscall* PFN_GetPauseState)(void* this_, uint8_t which);

uint8_t ReadCombatModeBit(void* clientLeader) {
    uint16_t v = 0;
    return acc::engine::TryReadU16(clientLeader, kCSWCCreatureCombatModeOffset, &v)
               ? static_cast<uint8_t>(v & 1) : 0xff;
}

int CallGetAutoPaused(void* exoApp) {
    if (!exoApp) return -1;
    __try {
        return static_cast<int>(
            reinterpret_cast<PFN_GetAutoPaused>(
                kAddrCClientExoAppGetAutoPaused)(exoApp));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int CallGetPauseState(void* exoApp) {
    if (!exoApp) return -1;
    __try {
        return static_cast<int>(
            reinterpret_cast<PFN_GetPauseState>(
                kAddrCClientExoAppGetPauseState)(exoApp, 0));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

struct State {
    int      cm  = 0xff;  // combat-mode bit (0/1, 0xff when unreadable)
    int      qs  = -1;    // inner queue size (combat_round.actions)
    int      oq  = -1;    // outer queue size (object.action_nodes): where
                          // chained presses stack, and what the strip UI shows
    int      ap  = -1;    // auto-paused
    int      ps  = -1;    // pause-state(0)
    uint32_t tgt = 0;     // main_interface target handle
};

State ReadState() {
    State s;
    void* server = acc::engine::GetPlayerServerCreature();
    void* exoApp = acc::engine::GetClientApp();
    s.cm = ReadCombatModeBit(acc::engine::GetClientLeader());
    s.qs = acc::combat::queue::ReadQueueSize(server);
    s.oq = acc::combat::queue::ReadOuterQueueSize(server);
    s.ap = CallGetAutoPaused(exoApp);
    s.ps = CallGetPauseState(exoApp);
    if (exoApp) {
        acc::engine::TryReadU32(acc::engine::ResolveMainInterface(),
                                kMainInterfaceTargetHandleOff, &s.tgt);
    }
    return s;
}

void LogState(const char* phase, const char* label, const State& s) {
    acclog::Write("Combat.Diag", "%s%s cm=%u qs=%d oq=%d ap=%d ps=%d tgt=0x%08x",
                  phase, label, s.cm, s.qs, s.oq, s.ap, s.ps, s.tgt);
}

void LogDelta(const char* what, int before, int after) {
    if (before != after) {
        acclog::Write("Combat.Diag", "DELTA %s %d -> %d", what, before, after);
    }
}

State g_last;
bool  g_armed = false;

}  // namespace

void Tick() {
    // Not while loading: there is no player to read.
    Vector unused;
    if (!acc::engine::GetPlayerPosition(unused)) return;

    const State now = ReadState();
    if (!g_armed) {
        g_last  = now;
        g_armed = true;
        LogState("INIT", "", now);
        return;
    }

    LogDelta("combat_mode", g_last.cm, now.cm);
    LogDelta("queue_size", g_last.qs, now.qs);
    LogDelta("outer_queue", g_last.oq, now.oq);
    LogDelta("autopause", g_last.ap, now.ap);
    LogDelta("pause_state", g_last.ps, now.ps);
    if (now.tgt != g_last.tgt) {
        acclog::Write("Combat.Diag", "DELTA target 0x%08x -> 0x%08x",
                      g_last.tgt, now.tgt);
    }
    g_last = now;
}

void LogPreFire(const char* label)  { LogState("PRE  ", label, ReadState()); }
void LogPostFire(const char* label) { LogState("POST ", label, ReadState()); }

}  // namespace acc::combat_diag

extern "C" void __cdecl OnCombatRoundRemoveAllActions(void* this_combatRound) {
    // A clear of an empty round is not logged. The engine's next instruction
    // after the cut is TEST EAX,EAX / JZ: such a clear removes nothing, and
    // this hook exists to show the engine culling queued entries. It matters
    // because the engine re-clears every live combat round once per frame
    // while combat is torn down for a cutscene handover: on the run-up to the
    // turret sequence that was six rounds a frame for seven seconds, about
    // 360 log lines a second, each a formatted write and a flush under a lock
    // on the game thread.
    //
    // -1 is "could not read the count", not "empty": those are logged, or a
    // genuine clear would vanish whenever the read faults.
    const int actions = acc::combat::queue::ReadRoundActionCount(this_combatRound);
    if (actions == 0) return;

    acclog::Write("Combat.Diag",
        "CLEAR [%s] round=%p actions=%d",
        acc::combat::queue::RoleTag(this_combatRound),
        this_combatRound, actions);
}

extern "C" void __cdecl OnCombatRoundSetCurrentAction(void* this_combatRound,
                                                     void* esp_byte_addr) {
    uint16_t slot = 0;
    const unsigned action = acc::engine::TryReadU16(esp_byte_addr, 0, &slot)
                                ? (slot & 0xffu) : 0xffu;
    acclog::Write("Combat.Diag",
        "SETCUR [%s] round=%p byte=%u",
        acc::combat::queue::RoleTag(this_combatRound),
        this_combatRound, action);
}

extern "C" void __cdecl OnCombatRoundRemoveLastAction(void* this_combatRound) {
    acclog::Write("Combat.Diag",
        "REMLAST [%s] round=%p",
        acc::combat::queue::RoleTag(this_combatRound),
        this_combatRound);
}

// ============================================================================
// KOTOR 2 wrappers. The K2 combat-round methods are unoptimised EBP-frame
// functions; every cut fires right after the prologue with `this` still live
// in ECX and the params at [EBP+8]. The shared KOTOR 1 handlers take
// stack-SLOT ADDRESSES (the esp+N LEA contract), so the wrappers hand the
// frame slots' addresses through unchanged.
// ============================================================================

// CSWSCombatRound::RemoveAllActions @0x00590430 (tail-remove loop until
// empty + per-action free — K1 0x004D3770's contract), hooked at 0x00590436.
extern "C" __declspec(dllexport)
void __cdecl OnCombatRoundRemoveAllActionsK2(void* thisRound) {
    if (!acc::game::IsKotor2()) return;
    OnCombatRoundRemoveAllActions(thisRound);
}

// CSWSCombatRound::SetCurrentAction @0x005908A0 (one-line byte store at
// [round+0xb24] — K1 0x004D2B40's exact twin), hooked at 0x005908A4.
//   ECX = this, [EBP+8] = action byte.
extern "C" __declspec(dllexport)
void __cdecl OnCombatRoundSetCurrentActionK2(void* thisRound, void* ebp) {
    if (!acc::game::IsKotor2()) return;
    if (!ebp) return;
    OnCombatRoundSetCurrentAction(thisRound, static_cast<char*>(ebp) + 8);
}

// CSWSCombatRound::RemoveLastAction @0x005904C0 (single tail remove + free,
// returns 1 — K1 0x004D37B0's contract), hooked at 0x005904C6.
extern "C" __declspec(dllexport)
void __cdecl OnCombatRoundRemoveLastActionK2(void* thisRound) {
    if (!acc::game::IsKotor2()) return;
    OnCombatRoundRemoveLastAction(thisRound);
}

// KOTOR 2's client→server "use item" request writer @0x00879AF0 — the
// message (type 0x15 on the (6,9) channel) every INVENTORY-item use from the
// personal action bar serialises before the server builds the UseItem action.
// Both personal-item handlers (equipped-slot and inventory) funnel through it.
// Hooked at 0x00879AF9, after the prologue's `this` store, ECX still = this;
// args at [EBP+8]=item id, +0xC/+0x10 mode bytes, +0x14=user id,
// +0x18=target vector ptr.
//
// The log line serves the dead-medical-column investigation
// (docs/known-issues.md): a personal press that produces this line but no
// AddAction died SERVER-side (the request was sent and dropped); a press with
// no line died CLIENT-side in DoPersonalAction's silent gates. Pairs with
// ActionBar.Fire's entry dump on the same press.
extern "C" __declspec(dllexport)
void __cdecl OnUseItemRequestK2(void* thisPtr, void* ebp) {
    if (!acc::game::IsKotor2()) return;
    if (!ebp) return;
    uint32_t itemId = 0, mode = 0, sub = 0, userId = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    int haveTarget = 0;
    __try {
        unsigned char* f = static_cast<unsigned char*>(ebp);
        itemId = *reinterpret_cast<uint32_t*>(f + 0x08);
        mode   = *reinterpret_cast<uint32_t*>(f + 0x0c);
        sub    = *reinterpret_cast<uint32_t*>(f + 0x10);
        userId = *reinterpret_cast<uint32_t*>(f + 0x14);
        float* tgt = *reinterpret_cast<float**>(f + 0x18);
        if (tgt) {
            x = tgt[0]; y = tgt[1]; z = tgt[2];
            haveTarget = 1;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Diag.UseItemReq", "frame deref faulted (ebp=%p)", ebp);
        return;
    }
    acclog::Write("Diag.UseItemReq",
                  "SEND this=%p item=0x%08x mode=0x%x sub=0x%x user=0x%08x "
                  "target=%d (%.2f,%.2f,%.2f)",
                  thisPtr, itemId, mode, sub, userId, haveTarget, x, y, z);
    // Not only a log line: a sent request proves the press dispatched, even
    // when the action never lands in the combat round (a mine plants without
    // an AddAction and read as "Not possible." while visibly working). A
    // refused press never reaches this writer, so the no-op watch still
    // speaks for those.
    acc::combat::queue::CancelNoOpWatch();
}
