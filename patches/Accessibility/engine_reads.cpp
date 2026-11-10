// The read primitives every engine read goes through: guarded field reads,
// the downcast call, CExoString and TLK text. The one SEH layer.

#include "engine_reads.h"

#include <windows.h>
#include <cstring>

#include "log.h"

namespace acc::engine {

namespace {

typedef void* (__thiscall* PFN_Downcast)(void* this_);

}  // namespace

void* CallDowncast(void* control, int vtableIndex) {
    if (!control) return nullptr;
    __try {
        void** vtable = *reinterpret_cast<void***>(control);
        if (!vtable) return nullptr;
        auto fn = reinterpret_cast<PFN_Downcast>(vtable[vtableIndex]);
        if (!fn) return nullptr;
        return fn(control);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool ReadCExoString(void* base, size_t offset,
                    char* outBuf, size_t bufSize) {
    // The length field alone is not enough to trust the buffer pointer. A
    // freed or phantom control (e.g. the stale row of an EMPTY save/load list —
    // the KOTOR 2 crash of 2026-09-04, dump-confirmed s=0x12, len=0x19) holds a
    // CExoString whose data pointer is small non-null garbage while its length
    // reads as a plausible in-range value, so the `!s`/`len` checks pass and
    // the memcpy then faults reading unmapped memory. Guard the whole read
    // under SEH: this is the one low-level copy every text extraction funnels
    // through, so protecting it here covers every caller (menu readers, per-tick
    // monitors, announce drains) against a bad CExoString rather than each
    // needing its own guard. No object needs unwinding here, so __try is legal
    // (MSVC C2712).
    __try {
        auto* p = reinterpret_cast<unsigned char*>(base) + offset;
        const char* s   = *reinterpret_cast<const char**>(p);
        uint32_t    len = *reinterpret_cast<uint32_t*>(p + 4);
        if (!s || len == 0 || len >= bufSize) return false;
        memcpy(outBuf, s, len);
        outBuf[len] = '\0';
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (bufSize) outBuf[0] = '\0';
        return false;
    }
}

uint32_t ReadU32(void* base, size_t offset) {
    return *reinterpret_cast<uint32_t*>(
        reinterpret_cast<unsigned char*>(base) + offset);
}

// SEH-guarded single-field reads for possibly-dead engine pointers. A
// non-null control/panel pointer is NOT proof of life (KOTOR 2 tears panels
// down on paths KOTOR 1 does not — the GetControlCenter / FocusProbe crash
// class), so any read through a pointer the engine may have freed goes
// through one of these instead of a raw dereference.
bool TryReadU32(void* base, size_t offset, uint32_t* out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadU16(void* base, size_t offset, uint16_t* out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<uint16_t*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadPtr(void* base, size_t offset, void** out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<void**>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Out-arg: the engine copy-constructs `out` from a stack-local CExoString,
// allocating a fresh c_string via its own CRT. We copy the string into our
// caller's buffer, then deliberately leak `tmp.c_string` — calling the
// engine's CExoString destructor from our DLL would risk heap mismatch, and
// focus events are low-frequency enough that the leak is negligible across
// a session.
//
// Sanity bounds: we reject strref values that look invalid (-1, >0x100000)
// before invoking the engine, to reduce the rate of expected exceptions.
//
// The engine's GetSimpleString has its own SEH frame (it can raise on
// out-of-range / corrupt indices). A previous attempt to call it from inside
// a hook handler caused our patch to go silent partway through Options —
// presumably the engine's exception unwound through our trampoline and the
// framework disabled the hook on subsequent fires. We now wrap the call in
// __try/__except so any raised exception is contained.
bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize) {
    if (strref == 0 || strref == 0xFFFFFFFF) return false;
    if (strref > 0x100000) return false;  // KOTOR's TLK is well below this

    void* tlk = *reinterpret_cast<void**>(kAddrTlkTablePtr);
    if (!tlk) return false;

    auto fn = reinterpret_cast<PFN_GetSimpleString>(kAddrGetSimpleString);
    CExoString tmp = {nullptr, 0};
    bool ok = false;
    __try {
        fn(tlk, &tmp, strref);
        if (tmp.c_string && tmp.length > 0 && tmp.length < bufSize) {
            memcpy(outBuf, tmp.c_string, tmp.length);
            outBuf[tmp.length] = '\0';
            ok = true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads", "TLK lookup raised SEH exception for strref=%u", strref);
        ok = false;
    }
    return ok;
}

bool ExtractTextOrStrRef(void* control,
                         size_t cexoOffset, size_t strRefOffset,
                         char* outBuf, size_t bufSize) {
    if (ReadCExoString(control, cexoOffset, outBuf, bufSize)) return true;
    uint32_t strref = ReadU32(control, strRefOffset);
    return LookupTlk(strref, outBuf, bufSize);
}

}  // namespace acc::engine
