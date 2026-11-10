// Hit points, Force points, effects, damage level and plot flag of creatures
// and objects.

#include "engine_reads.h"

#include <windows.h>
#include <algorithm>
#include <cstddef>

#include "engine_game.h"  // IsKotor2: ReadObjectDamageLevel branches per game

namespace acc::engine {

namespace {

typedef int (__thiscall* PFN_DamageLevel)(void* this_);
typedef int (__thiscall* PFN_GetHpThisInt)(void* this_, int param);

// K2 witnessed in its character-sheet SetStats twin 0x0084E6F0: "%d/%d" from
// the short at stats+0x4c, and its max-HP accessor 0x00848CF0 is a one-line
// read of the short at stats+0x4e. Both as in KOTOR 1.
const size_t kClientStatsCurrentHpOffset = acc::off::Same(0x4c);
const size_t kClientStatsMaxHpOffset     = acc::off::Same(0x4e);

// The CSWCCreatureStats block of a client creature; null on fault.
void* ClientCreatureStats(void* clientCreature) {
    void* stats = nullptr;
    TryReadPtr(clientCreature, kClientCreatureLevelUpStatsOffset, &stats);
    return stats;
}

// The u16 at fieldOffset of each non-null element of a pointer array.
// Returns the count written; stops at the first fault.
int ReadU16OfEach(void* data, int count, size_t fieldOffset, int* out) {
    int written = 0;
    for (int i = 0; i < count; ++i) {
        void* element = nullptr;
        uint16_t value = 0;
        if (!TryReadPtr(data, i * sizeof(void*), &element)) break;
        if (!element) continue;
        if (!TryReadU16(element, fieldOffset, &value)) break;
        out[written++] = value;
    }
    return written;
}

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

    const int cur = ReadCreatureCurrentHitPoints(serverObject);
    const int max = ReadCreatureMaxHitPoints(serverObject);
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
    uint16_t hp = 0;
    if (!TryReadU16(serverObject, kObjectHitPointsOffset, &hp)) return -1;
    return static_cast<short>(hp);
}

bool ReadClientCreatureHitPoints(void* clientCreature, int* outCur, int* outMax) {
    void* stats = ClientCreatureStats(clientCreature);
    uint16_t cur = 0, max = 0;
    if (!stats ||
        !TryReadU16(stats, kClientStatsCurrentHpOffset, &cur) ||
        !TryReadU16(stats, kClientStatsMaxHpOffset, &max)) {
        return false;
    }
    *outCur = static_cast<short>(cur);
    *outMax = static_cast<short>(max);
    return true;
}

bool ReadCreatureForcePoints(void* clientCreature, int* outCur, int* outMax) {
    if (outCur) *outCur = 0;
    if (outMax) *outMax = 0;

    // In the stats block: the cached max (live), then the base term (not
    // the live pool), then two shorts whose sum is the live current pool, as
    // CSWGuiInGameCharacter::SetStats (K1 0x006afda0, K2 0x0084E6F0) computes
    // the FP label. The server stats struct sits 2 bytes off this client one.
    // K2 moved this band by +8 (the HP fields did not move).
    const size_t kMaxForceOffset   = acc::off::Pick(0x11e, 0x126);
    const size_t kCurForceLoOffset = acc::off::Pick(0x122, 0x12a);
    const size_t kCurForceHiOffset = acc::off::Pick(0x124, 0x12c);

    void* stats = ClientCreatureStats(clientCreature);
    uint16_t maxFp = 0, curLo = 0, curHi = 0;
    if (!stats ||
        !TryReadU16(stats, kMaxForceOffset, &maxFp) ||
        !TryReadU16(stats, kCurForceLoOffset, &curLo) ||
        !TryReadU16(stats, kCurForceHiOffset, &curHi)) {
        return false;
    }
    if (outMax) *outMax = static_cast<int>(static_cast<short>(maxFp));
    // As the engine: (short)(lo + hi).
    if (outCur) {
        *outCur = static_cast<int>(static_cast<short>(
            static_cast<short>(curLo) + static_cast<short>(curHi)));
    }
    return true;
}

int ReadCreatureEffectIconIds(void* serverCreature, int* outIds, int cap) {
    void* data = nullptr;
    uint32_t size = 0;
    if (!TryReadPtr(serverCreature, kCreatureEffectIconsDataOffset, &data) || !data ||
        !TryReadU32(serverCreature, kCreatureEffectIconsSizeOffset, &size) ||
        size > static_cast<uint32_t>(cap)) {
        return 0;
    }
    return ReadU16OfEach(data, static_cast<int>(size), kEffectIconObjectIdOffset, outIds);
}

int ReadObjectEffectTypes(void* serverObject, int* outTypes, int cap) {
    void* data = nullptr;
    uint32_t size = 0;
    if (!TryReadPtr(serverObject, kObjectEffectsOffset, &data) || !data ||
        !TryReadU32(serverObject, kObjectEffectsOffset + offsetof(CExoArrayList, size),
                    &size) ||
        static_cast<int>(size) <= 0) {
        return 0;
    }
    const int count = (std::min)(static_cast<int>(size), cap);
    return ReadU16OfEach(data, count, kGameEffectTypeOffset, outTypes);
}

bool IsPlotObject(void* serverObject) {
    uint32_t plot = 0;
    return TryReadU32(serverObject, kObjectPlotOffset, &plot) && plot != 0;
}

}  // namespace acc::engine
