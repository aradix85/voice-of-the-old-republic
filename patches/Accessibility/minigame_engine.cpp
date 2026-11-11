#include "minigame_engine.h"

#include <windows.h>

#include "engine_app.h"     // GetClientAppInternal
#include "engine_area.h"

namespace acc::minigame {

using acc::engine::ReadVirtual;

void* ReadAreaMiniGame() {
    void* serverArea = acc::engine::GetCurrentArea();
    if (!serverArea) return nullptr;
    return ReadPtrOrNull(acc::engine::GetClientArea(serverArea), kClientAreaMiniGameOffset);
}

void* ResolveMgoArray() {
    return ReadPtrOrNull(acc::engine::GetClientAppInternal(), kClientInternalMgoArrayOffset);
}

void* ReadFollowerModelGob(void* follower) {
    // The models list is a CExoArrayList of pointers to model wrappers; a null
    // first element is the empty list, so its size never has to be read.
    return ReadPtrOrNull(ReadPtrOrNull(follower, kTrackFollowerModelsDataOffset), 0);
}

bool ReadFollowerPosition(void* follower, Vector& out) {
    typedef Vector* (__thiscall* PFN_GetPositionThunk)(void* this_, Vector* outBuf);
    void* model = ReadFollowerModelGob(follower);
    const auto getPosition = reinterpret_cast<PFN_GetPositionThunk>(
        ReadVirtual(model, kModelVtableSlotGetPosition));
    if (!getPosition) return false;
    __try {
        Vector buf = {0.0f, 0.0f, 0.0f};
        const Vector* ret = getPosition(model, &buf);
        out = ret ? *ret : buf;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadGobBoundingBox(void* gob, Vector& mn, Vector& mx) {
    typedef void (__thiscall* PFN_GetBBox)(void* this_, Vector* mn, Vector* mx);
    const auto getBox = reinterpret_cast<PFN_GetBBox>(
        ReadVirtual(gob, kGobVtableSlotGetBoundingBox));
    if (!getBox) return false;
    __try {
        Vector a = {0.0f, 0.0f, 0.0f}, b = {0.0f, 0.0f, 0.0f};
        getBox(gob, &a, &b);
        if (!(a.x <= b.x && a.y <= b.y && a.z <= b.z)) return false;  // NaN / empty
        mn = a; mx = b;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadGobPartBoundingBox(void* gob, const char* partName, Vector& mn, Vector& mx) {
    typedef void* (__thiscall* PFN_GetPart)(void* this_, const char* name);
    typedef void (__thiscall* PFN_PartMinBBox)(void* this_, Vector* mn, Vector* mx,
                                               float scale);
    if (!partName) return false;
    const auto getPart = reinterpret_cast<PFN_GetPart>(
        ReadVirtual(gob, kGobVtableSlotGetPart));
    if (!getPart) return false;
    __try {
        void* part = getPart(gob, partName);
        const auto minBox = reinterpret_cast<PFN_PartMinBBox>(
            ReadVirtual(part, kPartVtableSlotGetMinBBox));
        if (!minBox) return false;
        // The engine folds vertices into the caller's running min/max.
        Vector a = { 3.4e38f,  3.4e38f,  3.4e38f};
        Vector b = {-3.4e38f, -3.4e38f, -3.4e38f};
        minBox(part, &a, &b, 1.0f);
        if (!(a.x <= b.x && a.y <= b.y && a.z <= b.z)) return false;  // untouched / NaN
        if (a.x < -1.0e6f || b.x > 1.0e6f) return false;              // still the sentinels
        mn = a; mx = b;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadOffsetVector(void* player, Vector& out) {
    return TryReadVector(player, kMiniPlayerOffsetVectorOffset, &out);
}

void WriteOffsetVector(void* player, const Vector& v) {
    if (!player) return;
    __try {
        *reinterpret_cast<Vector*>(reinterpret_cast<unsigned char*>(player) +
                                   kMiniPlayerOffsetVectorOffset) = v;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

}  // namespace acc::minigame
