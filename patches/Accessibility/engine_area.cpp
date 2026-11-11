// Game objects and areas: the server object array, handle resolution
// (server and client side), object kind, position and tag, area and room
// names, and the iterator over an area's objects.

#include "engine_area_internal.h"

#include "engine_game.h"    // IsKotor2: GetRoom's K2 shape
#include "engine_player.h"  // GetPlayerArea
#include "engine_reads.h"   // ExtractTextOrStrRef, ReadCExoString

namespace acc::engine {

namespace {

typedef void* (__thiscall* PFN_CSWSAreaGetRoom)(void* this_,
                                                Vector* pos,
                                                int* outRoomIndex);
// KOTOR 2's GetRoom twin grew a parameter (found after the Peragus fuel-depot
// door-diag crash): (pos, outAux, inOutRoomIndex). outAux gets a
// hit detail from the ray intersect (nullable — the engine null-checks it);
// inOutRoomIndex is read as a try-this-room-first HINT before the full scan
// and written with the found index. Calling it with the 2-arg K1 shape left
// the hint slot as stack garbage: usually that garbage pointer faulted in
// the engine and the SEH catch in GetRoomAtIndexed cleaned up (which is why
// K2 never resolved a room, every caller saw -1) — but when the garbage
// happened to be readable the call returned NORMALLY with the stack off by
// 4 (thiscall callee cleanup: ret 0xC vs 8 pushed), and the wrapper's
// epilogue restored the caller's saved registers from shifted slots. That
// corrupted register was the door-diag crash at load on Peragus.
typedef void* (__thiscall* PFN_CSWSAreaGetRoomK2)(void* this_,
                                                  Vector* pos,
                                                  int* outAux,
                                                  int* inOutRoomIndex);
typedef void* (__thiscall* PFN_GetObjectArray)(void* this_);
typedef bool  (__thiscall* PFN_GetGameObject)(void* this_,
                                              uint32_t id,
                                              void** out);

// CClientExoApp::GetGameObject — direct one-call resolver. Returns
// CSWCObject* (client side); caller chains through +0xf8 to reach the
// matching server CSWSObject* the rest of engine_area expects.
typedef void* (__thiscall* PFN_CClientGetGameObject)(void* this_,
                                                    uint32_t handle);

// CServerExoApp → GetObjectArray() → CGameObjectArray*. The walk to the
// server app is guarded inside GetServerApp(); the __try here covers the
// engine call, which can still fault during teardown / very early init.
void* GetServerObjectArray() {
    void* serverApp = GetServerApp();
    if (!serverApp) return nullptr;
    __try {
        auto fn = reinterpret_cast<PFN_GetObjectArray>(
            kAddrCServerExoAppGetObjectArray);
        return fn(serverApp);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// CGameObjectArray::GetGameObject returns *false on hit, true on miss*: the
// "no match" branch falls through to `return true` after writing NULL to the
// out-param (decompiled @0x004d8230).
void* LookupGameObject(void* objectArray, uint32_t id) {
    auto resolve = reinterpret_cast<PFN_GetGameObject>(
        kAddrCGameObjectArrayGetGameObject);
    void* out = nullptr;
    bool miss = true;
    __try {
        miss = resolve(objectArray, id, &out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return (!miss && out) ? out : nullptr;
}

}  // namespace

void* GetCurrentArea() {
    return GetPlayerArea();  // already SEH-guarded inside engine_player
}

int GetObjectKind(void* gameObject) {
    uint8_t kind = 0;
    return TryReadU8(gameObject, kObjectKindOffset, &kind) ? kind : -1;
}

uint32_t GetObjectHandle(void* gameObject) {
    if (!gameObject) return 0;
    __try {
        // CGameObject.id @+0x4 (per /KotOR Types/Other Classes/CGameObject
        // size 0xc, members vtable@0x0 / id@0x4 / object_type@0x8). Same
        // ulong namespace AreaObjectIterator yields and ResolveServerObjectHandle
        // accepts.
        return *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(gameObject) + 0x4);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// Engine sentinels shared by both resolve paths: 0 (uninitialised),
// 0xFFFFFFFF (removed), 0x7F000000 (kInvalidObjectId — the "no object"
// marker the action queue and LastTarget use).
bool IsSentinelHandle(uint32_t handle) {
    return handle == 0u || handle == 0xFFFFFFFFu || handle == kInvalidObjectId;
}

void* ResolveServerObjectHandle(uint32_t handle) {
    if (IsSentinelHandle(handle)) return nullptr;

    void* objectArray = GetServerObjectArray();
    return objectArray ? LookupGameObject(objectArray, handle) : nullptr;
}

void* ResolveClientObject(uint32_t handle) {
    if (IsSentinelHandle(handle)) return nullptr;

    void* clientApp = GetClientApp();
    if (!clientApp) return nullptr;

    __try {
        auto fn = reinterpret_cast<PFN_CClientGetGameObject>(
            kAddrCClientExoAppGetGameObject);
        return fn(clientApp, handle);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void* ResolveClientObjectHandle(uint32_t handle) {
    void* clientObject = ResolveClientObject(handle);
    if (!clientObject) return nullptr;

    // CSWCObject.server_object @+0xf8 → CSWSObject*. Same offset
    // engine_player uses on the player creature's CSWCCreature.
    return ReadPtrOrNull(clientObject, kClientObjectServerObjectOffset);
}

bool GetObjectPosition(void* gameObject, Vector& out) {
    return TryReadVector(gameObject, kServerObjectPositionOffset, &out);
}

void* GetRoomAtIndexed(void* area, const Vector& pos, int& outIndex) {
    outIndex = -1;
    if (!area) return nullptr;
    Vector local = pos;
    __try {
        // Per-game signatures — see the PFN_CSWSAreaGetRoomK2 note above.
        // outIndex doubles as K2's in/out hint: -1 in (skips the hint
        // path), found index out, untouched on miss.
        if (acc::game::IsKotor2()) {
            auto fn = reinterpret_cast<PFN_CSWSAreaGetRoomK2>(
                kAddrCSWSAreaGetRoom);
            return fn(area, &local, nullptr, &outIndex);
        }
        auto fn = reinterpret_cast<PFN_CSWSAreaGetRoom>(kAddrCSWSAreaGetRoom);
        return fn(area, &local, &outIndex);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outIndex = -1;
        return nullptr;
    }
}

bool GetAreaDisplayName(void* area, char* outBuf, size_t bufSize) {
    if (!area || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    // Localized name first: CExoLocString shape matches CExoString — try
    // the inline c_string at +0x150, fall back to TLK strref at +0x154.
    if (ExtractTextOrStrRef(area, kAreaNameLocOffset,
                            kAreaNameLocOffset + 4, outBuf, bufSize) &&
        outBuf[0] != '\0') {
        return true;
    }
    // Modder-assigned tag (CExoString at +0x158): "tar_m02ac" beats silence.
    return ReadCExoString(area, kAreaTagOffset, outBuf, bufSize);
}

bool GetObjectTag(void* gameObject, char* outBuf, size_t bufSize) {
    if (!gameObject || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    return ReadCExoString(gameObject, kObjectTagOffset, outBuf, bufSize);
}

bool GetAreaTag(void* area, char* outBuf, size_t bufSize) {
    if (!area || !outBuf || bufSize < 2) return false;
    outBuf[0] = '\0';
    return ReadCExoString(area, kAreaTagOffset, outBuf, bufSize);
}

bool GetRoomDisplayName(void* area, int roomIndex,
                        char* outBuf, size_t bufSize) {
    if (!area || !outBuf || bufSize < 2 || roomIndex < 0) return false;
    outBuf[0] = '\0';
    __try {
        auto* base = reinterpret_cast<unsigned char*>(area);
        int roomCount = static_cast<int>(*reinterpret_cast<uint32_t*>(
            base + kAreaRoomCountOffset));
        if (roomIndex >= roomCount) return false;
        void* namesArray = *reinterpret_cast<void**>(
            base + kAreaRoomNamesOffset);
        if (!namesArray) return false;
        size_t entryOffset =
            static_cast<size_t>(roomIndex) * kCExoStringStride;
        return ReadCExoString(namesArray, entryOffset, outBuf, bufSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

AreaObjectIterator::AreaObjectIterator(void* area)
    : handles_(nullptr), size_(0), index_(0), objectArray_(nullptr) {
    if (!area) return;
    __try {
        auto* base = reinterpret_cast<unsigned char*>(area);
        handles_ = *reinterpret_cast<uint32_t**>(base + kAreaGameObjectsOffset);
        size_    = *reinterpret_cast<int*>     (base + kAreaGameObjectCountOffset);
        if (size_ < 0) size_ = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        handles_ = nullptr;
        size_    = 0;
    }
    objectArray_ = GetServerObjectArray();
}

void* AreaObjectIterator::Next() {
    if (!handles_ || !objectArray_) return nullptr;
    while (index_ < size_) {
        uint32_t id = 0;
        __try {
            id = handles_[index_++];
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            size_ = 0;
            return nullptr;
        }
        if (IsSentinelHandle(id)) continue;
        if (void* out = LookupGameObject(objectArray_, id)) return out;
    }
    return nullptr;
}

}  // namespace acc::engine
