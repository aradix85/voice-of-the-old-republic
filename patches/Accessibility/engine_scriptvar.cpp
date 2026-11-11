// The player's script variables: the engine accessors behind ReadPlayerVar
// and WritePlayerVar, and the settle wait. The model and the save / reload
// proof are in docs/llm-docs/persistence-scriptvartable.md.

#include "engine_scriptvar.h"

#include <windows.h>
#include <cstdint>
#include <cstring>

#include "engine_player.h"   // GetPlayerServerCreature
#include "engine_reads.h"    // ReadCExoString
#include "log.h"
#include "engine_addr.h"
#include "engine_offsets_select.h"

namespace acc::engine {
namespace {

// CSWSScriptVarTable accessors + CExoString ctor/dtor (GOG/Steam swkotor.exe;
// GoG bytes match Steam). Addresses from Lane's RE — see the persistence doc.
// K2 CSWSScriptVarTable accessors — witnessed in the var-table cluster
// (0x005e6580 is the shared name-lookup; the four thin accessors wrap it with
// the K1 (type, create) pair: GetInt lookup(1,0), GetString lookup(3,0),
// SetInt lookup(1,1), SetString lookup(3,1)).
const uintptr_t kAddrSetString = acc::addr::Pick(0x0059a8e0, 0x005e6ce0);  // __thiscall(table, name, value)
const uintptr_t kAddrGetString = acc::addr::Pick(0x0059a590, 0x005e6850);  // __thiscall(table, out, name) -> out
const uintptr_t kAddrExoCtorCStr = acc::addr::Pick(0x005e5a90, 0x00733570);  // CExoString::CExoString(char*)
const uintptr_t kAddrExoDtor = acc::addr::Pick(0x005e5c20, 0x00733780);  // CExoString::~CExoString()

// Named, string-capable CSWSScriptVarTable embedded in CSWSObject at +0x100.
// (Ghidra's struct MISLABELS the fields: the symbol "script_var_table_2" at
// +0x110 is actually a CSWVarTable — the fixed, index-keyed, NON-string table.
// CSWSObject::SaveObjectState/LoadObjectState @0x004cec50/0x004d1cf0 prove the
// real CSWSScriptVarTable is the +0x100 field — they call
// CSWSScriptVarTable::Save/LoadVarTable on &this->field54_0x100 and
// CSWVarTable::Save/LoadVarTable on &this->script_var_table_2. Writing through
// +0x110 with the string accessors derefs a bogus array pointer and faults.)
// A CSWSObject FIELD offset, despite the uintptr_t typing — so it takes an
// acc::off marker, not an address one. KOTOR 2 value unknown: CSWSObject's
// shallow fields shift +4 there, but +0x100 is far enough down the class that
// the shift cannot be assumed to be the same. Verify before trusting saved
// mod state on KOTOR 2.
// K2 +0x104: witnessed in the object serializer thunk (0x00540660) calling
// CSWSScriptVarTable::LoadVarTable on `this + 0x104` (the fixed CSWVarTable
// LoadVarTable follows at this + 0x114 → kObjectVarTableOffset).
const size_t kScriptVarTableOffset = acc::off::Pick(0x100, 0x104);

typedef void  (__thiscall* PFN_SetString)(void*, void*, void*);
// GetString returns CExoString by value → the hidden out-buffer pointer is the
// FIRST stack arg, `name` second (returned in EAX). Declared exactly so the
// call doesn't return caller-frame garbage (see the persistence doc's
// calling-convention trap).
typedef void* (__thiscall* PFN_GetString)(void*, void*, void*);
typedef void* (__thiscall* PFN_ExoCtorCStr)(void*, const char*);
typedef void  (__thiscall* PFN_ExoDtor)(void*);

// Engine CExoString layout: { char* c_string; int length } (8 bytes). POD so
// it is legal as a local inside an SEH (__try) frame.
struct ExoStr { char* p; int len; };

void ExoInit(ExoStr* s, const char* cstr) {
    s->p = nullptr;
    s->len = 0;
    reinterpret_cast<PFN_ExoCtorCStr>(kAddrExoCtorCStr)(s, cstr);
}

void ExoFree(ExoStr* s) {
    reinterpret_cast<PFN_ExoDtor>(kAddrExoDtor)(s);
}

// About a second at 60 fps: long after the table is filled, well before the
// player could do anything that reads it.
constexpr int kSettleTicks = 60;

// creature+0x100, or nullptr when no server-side player creature is loaded.
void* PlayerVarTable() {
    // acc::off::Ptr, not raw addition: kScriptVarTableOffset is still Todo on
    // KOTOR 2, and a poisoned offset must yield nullptr rather than a non-null
    // wild pointer (see acc::off::Ptr's note).
    return acc::off::Ptr(GetPlayerServerCreature(), kScriptVarTableOffset);
}

// Reads into outBuf, always NUL-terminated; true for a non-empty value.
bool GetPlayerVarString(const char* name, char* outBuf, size_t bufSize) {
    if (outBuf && bufSize > 0) outBuf[0] = '\0';
    void* table = PlayerVarTable();
    if (!table || !name || !outBuf || bufSize == 0) return false;
    __try {
        ExoStr n; ExoInit(&n, name);
        // `out` starts as a valid empty CExoString; GetString's internal
        // operator= frees this (no-op for {null,0}) before copying. We then
        // free the engine-allocated buffer with the engine's own dtor (no heap
        // mismatch) — the proven scriptvar_poc recipe.
        ExoStr out = {nullptr, 0};
        reinterpret_cast<PFN_GetString>(kAddrGetString)(table, &out, &n);
        bool ok = ReadCExoString(&out, 0, outBuf, bufSize);
        ExoFree(&out);
        ExoFree(&n);
        return ok;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (outBuf && bufSize > 0) outBuf[0] = '\0';
        return false;
    }
}

bool SetPlayerVarString(const char* name, const char* value) {
    void* table = PlayerVarTable();
    if (!table || !name || !value) return false;
    __try {
        ExoStr n; ExoInit(&n, name);
        ExoStr v; ExoInit(&v, value);
        reinterpret_cast<PFN_SetString>(kAddrSetString)(table, &n, &v);
        ExoFree(&v);
        ExoFree(&n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

std::string ReadPlayerVar(const std::string& name) {
    static char buf[16384];
    return GetPlayerVarString(name.c_str(), buf, sizeof(buf)) ? buf : "";
}

bool WritePlayerVar(const std::string& name, const std::string& value) {
    return SetPlayerVarString(name.c_str(), value.c_str());
}

void PlayerVarsSettle::Reset() {
    creature_ = nullptr;
    ticks_    = 0;
}

bool PlayerVarsSettle::Tick() {
    void* creature = GetPlayerServerCreature();
    if (creature != creature_) {
        creature_ = creature;
        ticks_    = 0;
    }
    return creature && ++ticks_ >= kSettleTicks;
}

}  // namespace acc::engine
