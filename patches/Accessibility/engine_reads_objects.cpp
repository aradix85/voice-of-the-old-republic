// Hit points, damage level and plot flag of server objects.

#include "engine_reads.h"

#include <windows.h>

#include "engine_game.h"  // IsKotor2: ReadObjectDamageLevel branches per game

namespace acc::engine {

namespace {

typedef int (__thiscall* PFN_DamageLevel)(void* this_);
typedef int (__thiscall* PFN_GetHpThisInt)(void* this_, int param);

}  // namespace

// KOTOR 1: CSWSObject::GetDamageLevel @0x004cb020. The return is ulong but
// only AL holds the 0..5 bucket; buckets 0..3 leave comparison-flag garbage
// in the upper bytes (decompile note at the address declaration), so mask
// to the low byte before the range check.
//
// KOTOR 2: no located twin (see kAddrCSWSObjectGetDamageLevel), so this is
// a replica of the decompiled K1 body: ratio = GetCurrentHitPoints(0) /
// GetMaxHitPoints(1), both as signed shorts, bucketed at >=0.95 / >=0.75 /
// >=0.5 / >=0.25 / >0. Param 0 on the current-HP read means current PLUS
// temporary hit points — the exact term the K1 original feeds the ratio.
int ReadObjectDamageLevel(void* serverObject) {
    if (!serverObject) return -1;

    if (!acc::game::IsKotor2()) {
        __try {
            auto fn = reinterpret_cast<PFN_DamageLevel>(
                kAddrCSWSObjectGetDamageLevel);
            int v = fn(serverObject) & 0xFF;
            if (v < 0 || v > 5) return -1;
            return v;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return -1;
        }
    }

    int cur = 0;
    int max = 0;
    __try {
        auto curFn = reinterpret_cast<PFN_GetHpThisInt>(
            kAddrCSWSObjectGetCurrentHitPoints);
        auto maxFn = reinterpret_cast<PFN_GetHpThisInt>(
            kAddrCSWSCreatureGetMaxHitPoints);
        cur = static_cast<short>(curFn(serverObject, 0));
        max = static_cast<short>(maxFn(serverObject, 1));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    if (max <= 0 || max > 0x4000) return -1;

    float ratio = static_cast<float>(cur) / static_cast<float>(max);
    if (ratio >= 0.95f) return 0;
    if (ratio >= 0.75f) return 1;
    if (ratio >= 0.5f)  return 2;
    if (ratio >= 0.25f) return 3;
    if (ratio > 0.0f)   return 4;
    return 5;
}

int ReadCreatureCurrentHitPoints(void* serverCreature) {
    if (!serverCreature) return -1;
    __try {
        auto curFn = reinterpret_cast<PFN_GetHpThisInt>(
            kAddrCSWSObjectGetCurrentHitPoints);
        return static_cast<short>(curFn(serverCreature, 0));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int ReadCreatureMaxHitPoints(void* serverCreature) {
    if (!serverCreature) return -1;
    __try {
        auto maxFn = reinterpret_cast<PFN_GetHpThisInt>(
            kAddrCSWSCreatureGetMaxHitPoints);
        return static_cast<short>(maxFn(serverCreature, 1));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int ReadObjectCurrentHitPoints(void* serverObject) {
    if (!serverObject) return -1;
    __try {
        return static_cast<int>(*reinterpret_cast<short*>(
            reinterpret_cast<unsigned char*>(serverObject) +
            kObjectHitPointsOffset));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool IsPlotObject(void* serverObject) {
    if (!serverObject) return false;
    __try {
        return *reinterpret_cast<uint32_t*>(
                   reinterpret_cast<unsigned char*>(serverObject) +
                   kObjectPlotOffset) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace acc::engine
