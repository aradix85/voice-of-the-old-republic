# prism_internal.h (82 lines)

Source: `patches/Accessibility/prism_internal.h`

Internal to the prism module (prism*.cpp); no other file includes it. The public interface is prism.h. Here: what the files share.

## Declarations (in source order)

- L18 `typedef struct PrismContext PrismContext`: Subset of Prism's C ABI we use.
- L19 `typedef struct PrismBackend PrismBackend`
- L20 `typedef uint64_t PrismBackendId`
- L21 `typedef int PrismError`
- L32 `inline bool InitOk(PrismError rc)`: A backend that reports ALREADY_INITIALIZED is a cached, working instance.
- L40 `typedef PrismContext* (__cdecl* PFN_prism_init)(void*)`: prism_init takes a void* and is always handed nullptr.
- L41 `typedef size_t (__cdecl* PFN_prism_registry_count)(PrismContext*)`
- L42 `typedef PrismBackendId (__cdecl* PFN_prism_registry_id_at)(PrismContext*, size_t)`
- L43 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire)(PrismContext*, PrismBackendId)`
- L44 `typedef PrismBackend* (__cdecl* PFN_prism_registry_acquire_best)(PrismContext*)`
- L45 `typedef uint64_t (__cdecl* PFN_prism_backend_get_features)(PrismBackend*)`
- L46 `typedef PrismError (__cdecl* PFN_prism_backend_initialize)(PrismBackend*)`
- L47 `typedef PrismError (__cdecl* PFN_prism_backend_speak)(PrismBackend*, const char*, bool)`
- L48 `typedef PrismError (__cdecl* PFN_prism_backend_output)(PrismBackend*, const char*, bool)`
- L49 `typedef PrismError (__cdecl* PFN_prism_backend_braille)(PrismBackend*, const char*)`
- L50 `typedef PrismError (__cdecl* PFN_prism_backend_stop)(PrismBackend*)`
- L51 `typedef const char* (__cdecl* PFN_prism_backend_name)(PrismBackend*)`
- L76 `std::string ToUtf8(const char* gameText)`: prism_text.cpp
- L77 `std::string WideToUtf8(const wchar_t* text)`
- L78 `void CaptureSpoken(const char* text)`
- L79 `void CaptureSpokenW(const wchar_t* text)`

Also 19 constants or variables at namespace scope.
