// Internal to the prism module (prism*.cpp); no other file includes
// it. The public interface is prism.h. Here: what the files share.

#pragma once

#include "prism.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace prism {

// Subset of Prism's C ABI we use. Matches third_party/prism-dist/include/prism.h
// verbatim. We declare the function-pointer types here rather than including
// prism.h so the patch DLL build has no header dependency on the upstream tree.
typedef struct PrismContext PrismContext;
typedef struct PrismBackend PrismBackend;
typedef uint64_t PrismBackendId;
typedef int PrismError;  // 0 == PRISM_OK

constexpr PrismError kPrismOk                     = 0;
// The backend has no implementation of the entry point — e.g. braille on a
// speech-only backend, or ZDSR running against a ZDSRAPI that predates its
// Braille export.
constexpr PrismError kPrismErrNotImplemented      = 3;
constexpr PrismError kPrismErrAlreadyInitialized  = 15;  // see PrismError enum
constexpr PrismError kPrismErrUnknown             = 17;  // sentinel for SEH faults

// A backend that reports ALREADY_INITIALIZED is a cached, working instance.
inline bool InitOk(PrismError rc) {
    return rc == kPrismOk || rc == kPrismErrAlreadyInitialized;
}

// prism_init takes a void* and is always handed nullptr. It reads nothing from
// a NULL config and falls back to the global backend registry, which is exactly
// what we want — and that avoids declaring PrismConfig, whose layout changes
// between Prism versions in a way no single declaration survives.
typedef PrismContext*  (__cdecl* PFN_prism_init)(void*);
typedef size_t         (__cdecl* PFN_prism_registry_count)(PrismContext*);
typedef PrismBackendId (__cdecl* PFN_prism_registry_id_at)(PrismContext*, size_t);
typedef PrismBackend*  (__cdecl* PFN_prism_registry_acquire)(PrismContext*, PrismBackendId);
typedef PrismBackend*  (__cdecl* PFN_prism_registry_acquire_best)(PrismContext*);
typedef uint64_t       (__cdecl* PFN_prism_backend_get_features)(PrismBackend*);
typedef PrismError     (__cdecl* PFN_prism_backend_initialize)(PrismBackend*);
typedef PrismError     (__cdecl* PFN_prism_backend_speak)(PrismBackend*, const char*, bool);
typedef PrismError     (__cdecl* PFN_prism_backend_output)(PrismBackend*, const char*, bool);
typedef PrismError     (__cdecl* PFN_prism_backend_braille)(PrismBackend*, const char*);
typedef PrismError     (__cdecl* PFN_prism_backend_stop)(PrismBackend*);
typedef const char*    (__cdecl* PFN_prism_backend_name)(PrismBackend*);

// Init resolves every pointer before it sets g_prismCtx, and fails without
// one. Code that runs only with a context or a backend in hand calls them
// unchecked.
extern PFN_prism_init                  pPrism_init;
extern PFN_prism_registry_count        pPrism_registry_count;
extern PFN_prism_registry_id_at        pPrism_registry_id_at;
extern PFN_prism_registry_acquire      pPrism_registry_acquire;
extern PFN_prism_registry_acquire_best pPrism_registry_acquire_best;
extern PFN_prism_backend_get_features  pPrism_backend_get_features;
extern PFN_prism_backend_initialize    pPrism_backend_initialize;
extern PFN_prism_backend_speak         pPrism_backend_speak;
extern PFN_prism_backend_output        pPrism_backend_output;
extern PFN_prism_backend_braille       pPrism_backend_braille;
extern PFN_prism_backend_stop          pPrism_backend_stop;
extern PFN_prism_backend_name          pPrism_backend_name;

extern PrismContext* g_prismCtx;
extern PrismBackend* g_prismNormal;    // NVDA / JAWS / OneCore / ... — chosen by acquire_best
extern bool          g_normalReady;

// Helpers shared across the files, by the file that defines them.

// prism_text.cpp
std::string ToUtf8(const char* gameText);
std::string WideToUtf8(const wchar_t* text);
void CaptureSpoken(const char* text);
void CaptureSpokenW(const wchar_t* text);

}  // namespace prism
