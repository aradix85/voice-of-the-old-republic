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

void* ReadVirtual(void* object, size_t slotOffset) {
    return ReadPtrOrNull(reinterpret_cast<void*>(ReadVtable(object)), slotOffset);
}

PFN_GuiHandleInputEvent ReadHandleInputEvent(void* object) {
    return reinterpret_cast<PFN_GuiHandleInputEvent>(
        ReadVirtual(object, kVtableSlotHandleInputEvent * sizeof(void*)));
}

void* CallDowncast(void* object, int vtableIndex) {
    const auto downcast = reinterpret_cast<PFN_Downcast>(
        ReadVirtual(object, static_cast<size_t>(vtableIndex) * sizeof(void*)));
    if (!downcast) return nullptr;
    __try {
        return downcast(object);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool ReadCExoString(void* base, size_t offset,
                    char* outBuf, size_t bufSize) {
    // The copy is guarded too: a freed or phantom control (the stale row of
    // an empty save list) can hold a small garbage pointer next to a
    // plausible length, which passes both checks and faults in memcpy.
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

bool TryWriteU32(void* base, size_t offset, uint32_t value) {
    if (!base) return false;
    __try {
        *reinterpret_cast<uint32_t*>(reinterpret_cast<unsigned char*>(base) + offset) =
            value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryWriteU16(void* base, size_t offset, uint16_t value) {
    if (!base) return false;
    __try {
        *reinterpret_cast<uint16_t*>(reinterpret_cast<unsigned char*>(base) + offset) =
            value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryWriteFloat(void* base, size_t offset, float value) {
    if (!base) return false;
    __try {
        *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(base) + offset) =
            value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryWriteVector(void* base, size_t offset, const Vector& value) {
    if (!base) return false;
    __try {
        *reinterpret_cast<Vector*>(reinterpret_cast<unsigned char*>(base) + offset) =
            value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadU8(void* base, size_t offset, uint8_t* out) {
    if (!base || !out) return false;
    __try {
        *out = *(reinterpret_cast<unsigned char*>(base) + offset);
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

bool TryReadVector(void* base, size_t offset, Vector* out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<Vector*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

uintptr_t ReadVtable(void* object) {
    void* vtable = nullptr;
    TryReadPtr(object, 0, &vtable);
    return reinterpret_cast<uintptr_t>(vtable);
}

bool HasVtable(void* object, uintptr_t vtable) {
    return vtable != 0 && ReadVtable(object) == vtable;
}

bool TryReadFloat(void* base, size_t offset, float* out) {
    if (!base || !out) return false;
    __try {
        *out = *reinterpret_cast<float*>(
            reinterpret_cast<unsigned char*>(base) + offset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The engine copy-constructs the result into `tmp`, allocating c_string with
// its own CRT. That string is leaked on purpose: freeing it from this DLL
// risks a heap mismatch, and lookups are rare enough for the leak not to
// matter. GetSimpleString can raise on a corrupt index, and an exception
// that unwinds through a hook trampoline disables the hook, so the call is
// guarded and implausible strrefs are rejected before it.
bool LookupTlk(uint32_t strref, char* outBuf, size_t bufSize) {
    if (strref == 0 || strref == kNoStrref) return false;
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
    uint32_t strref = 0;
    return TryReadU32(control, strRefOffset, &strref) &&
           LookupTlk(strref, outBuf, bufSize);
}

}  // namespace acc::engine
